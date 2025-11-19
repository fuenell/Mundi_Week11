#include "pch.h"
#include "SSkeletalMeshViewerWindow.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/SkeletalViewer/SkeletalViewerBootstrap.h"
#include "Source/Editor/PlatformProcess.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Components/LineComponent.h"
#include "SelectionManager.h"
#include "USlateManager.h"
#include "BoneAnchorComponent.h"
#include "Source/Runtime/Engine/Collision/Picking.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Animation/AnimSingleNodeInstance.h"
#include "Source/Runtime/Animation/AnimSequence.h"
#include <cstring>
#include <algorithm>

namespace
{
	const TArray<UClass*>& GetAnimNotifyClassList()
	{
		static TArray<UClass*> AnimNotifyClasses;
		static bool bInitialized = false;

		if (!bInitialized)
		{
			UClass* BaseClass = UAnimNotify::StaticClass();
			for (UClass* Class : UClass::GetAllClasses())
			{
				if (Class && Class != BaseClass && Class->IsChildOf(BaseClass))
				{
					AnimNotifyClasses.Add(Class);
				}
			}

			bInitialized = true;
		}

		return AnimNotifyClasses;
	}
}

SSkeletalMeshViewerWindow::SSkeletalMeshViewerWindow()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SSkeletalMeshViewerWindow::~SSkeletalMeshViewerWindow()
{
    // Clean up tabs if any
    for (int i = 0; i < Tabs.Num(); ++i)
    {
        ViewerState* State = Tabs[i];
        ReleaseRenderTarget(State);
        SkeletalViewerBootstrap::DestroyViewerState(State);
    }
    Tabs.Empty();
    ActiveState = nullptr;
}

bool SSkeletalMeshViewerWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice)
{
    World = InWorld;
    Device = InDevice;
    
    SetRect(StartX, StartY, StartX + Width, StartY + Height);

    // Create first tab/state
    OpenNewTab("Viewer 1");
    if (ActiveState && ActiveState->Viewport)
    {
        ActiveState->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)Width, (uint32)Height);
    }

    bRequestFocus = true;
    return true;
}

void SSkeletalMeshViewerWindow::OnRender()
{
    // If window is closed, don't render
    if (!bIsOpen)
    {
        return;
    }

    // Parent detachable window (movable, top-level) with solid background
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }

    if (bRequestFocus)
    {
        ImGui::SetNextWindowFocus();
    }
    bool bViewerVisible = false;
    if (ImGui::Begin("Skeletal Mesh Viewer", &bIsOpen, flags))
    {
        bViewerVisible = true;
        // Render tab bar and switch active state
        if (ImGui::BeginTabBar("SkeletalViewerTabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable))
        {
            for (int i = 0; i < Tabs.Num(); ++i)
            {
                ViewerState* State = Tabs[i];
                bool open = true;
                if (ImGui::BeginTabItem(State->Name.ToString().c_str(), &open))
                {
                    ActiveTabIndex = i;
                    ActiveState = State;
                    ImGui::EndTabItem();
                }
                if (!open)
                {
                    CloseTab(i);
                    break;
                }
            }
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            {
                char label[32]; sprintf_s(label, "Viewer %d", Tabs.Num() + 1);
                OpenNewTab(label);
            }
            ImGui::EndTabBar();
        }
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x; Rect.Top = pos.y; Rect.Right = pos.x + size.x; Rect.Bottom = pos.y + size.y; Rect.UpdateMinMax();

        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        float totalWidth = contentAvail.x;
        
        // Calculate playback bar height based on animation mode
        const float PerTrackTimelineHeight = 40.0f;
        float FinalPlaybackBarHeight = 0.0f;
        if (ActiveState && ActiveState->bAnimationMode)
        {
            USkeletalMeshComponent* PlaybackMeshComp = ActiveState->PreviewActor ? ActiveState->PreviewActor->GetSkeletalMeshComponent() : nullptr;
            UAnimSingleNodeInstance* PlaybackInstance = PlaybackMeshComp ? PlaybackMeshComp->GetSingleNodeInstance() : nullptr;
            UAnimSequence* PlaybackAnim = PlaybackInstance ? PlaybackInstance->GetCurrentAnimSequence() : nullptr;
            int32 NotifyTrackCount = PlaybackAnim ? std::max(1, static_cast<int32>(PlaybackAnim->NotifyTracks.Num())) : 1;
            int32 VisibleTrackCountForHeight = std::min(NotifyTrackCount, 2);
            FinalPlaybackBarHeight = PlaybackBarBaseHeight + (VisibleTrackCountForHeight * PerTrackTimelineHeight);
        }
        float bottomBarsTotalHeight = AnimationModeCheckboxHeight + FinalPlaybackBarHeight;
        
        // Main panels height (subtract bottom bars from available height)
        float totalHeight = contentAvail.y - bottomBarsTotalHeight;

        float leftWidth = totalWidth * LeftPanelRatio;
        float rightWidth = totalWidth * RightPanelRatio;
        float centerWidth = totalWidth - leftWidth - rightWidth;

        // Remove spacing between panels
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // Left panel - Asset Browser & Bone Hierarchy
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();

        if (ActiveState)
        {
            // Asset Browser Section
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
            ImGui::Indent(8.0f);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::Text("Asset Browser");
            ImGui::PopFont();
            ImGui::Unindent(8.0f);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // Mesh path section
            ImGui::BeginGroup();
            ImGui::Text("Mesh Path:");
            ImGui::PushItemWidth(-1.0f);
            ImGui::InputTextWithHint("##MeshPath", "Browse for FBX file...", ActiveState->MeshPathBuffer, sizeof(ActiveState->MeshPathBuffer));
            ImGui::PopItemWidth();

            ImGui::Spacing();

            // Buttons
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.40f, 0.55f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.50f, 0.70f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.35f, 0.50f, 1.0f));

            float buttonWidth = (leftWidth - 24.0f) * 0.5f - 4.0f;
            if (ImGui::Button("Browse...", ImVec2(buttonWidth, 32)))
            {
                auto widePath = FPlatformProcess::OpenLoadFileDialog(UTF8ToWide(GDataDir), L"fbx", L"FBX Files");
                if (!widePath.empty())
                {
                    std::string s = widePath.string();
                    strncpy_s(ActiveState->MeshPathBuffer, s.c_str(), sizeof(ActiveState->MeshPathBuffer) - 1);
                }
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.40f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.70f, 0.50f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.50f, 0.30f, 1.0f));
            if (ImGui::Button("Load FBX", ImVec2(buttonWidth, 32)))
            {
                FString Path = ActiveState->MeshPathBuffer;
                if (!Path.empty())
                {
                    USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
                    if (Mesh && ActiveState->PreviewActor)
                    {
                        ActiveState->PreviewActor->SetSkeletalMesh(Path);
                        ActiveState->CurrentMesh = Mesh;
                        ActiveState->LoadedMeshPath = Path;
                        if (auto* Skeletal = ActiveState->PreviewActor->GetSkeletalMeshComponent())
                        {
                            Skeletal->SetVisibility(ActiveState->bShowMesh);
                        }
                        ActiveState->bBoneLinesDirty = true;
                        if (auto* LineComp = ActiveState->PreviewActor->GetBoneLineComponent())
                        {
                            LineComp->ClearLines();
                            LineComp->SetLineVisible(ActiveState->bShowBones);
                        }
                    }
                }
            }
            ImGui::PopStyleColor(6);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // Display Options
            ImGui::BeginGroup();
            ImGui::Text("Display Options:");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.30f, 0.35f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.40f, 0.70f, 1.00f, 1.0f));

            if (ImGui::Checkbox("Show Mesh", &ActiveState->bShowMesh))
            {
                if (ActiveState->PreviewActor && ActiveState->PreviewActor->GetSkeletalMeshComponent())
                {
                    ActiveState->PreviewActor->GetSkeletalMeshComponent()->SetVisibility(ActiveState->bShowMesh);
                }
            }

            ImGui::SameLine();
            if (ImGui::Checkbox("Show Bones", &ActiveState->bShowBones))
            {
                if (ActiveState->PreviewActor && ActiveState->PreviewActor->GetBoneLineComponent())
                {
                    ActiveState->PreviewActor->GetBoneLineComponent()->SetLineVisible(ActiveState->bShowBones);
                }
                if (ActiveState->bShowBones)
                {
                    ActiveState->bBoneLinesDirty = true;
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // Bone Hierarchy Section
            ImGui::Text("Bone Hierarchy:");
            ImGui::Spacing();

            if (!ActiveState->CurrentMesh)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("No skeletal mesh loaded.");
                ImGui::PopStyleColor();
            }
            else
            {
                const FSkeleton* Skeleton = ActiveState->CurrentMesh->GetSkeleton();
                if (!Skeleton || Skeleton->Bones.IsEmpty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::TextWrapped("This mesh has no skeleton data.");
                    ImGui::PopStyleColor();
                }
                else
                {
                    // Scrollable tree view
                    ImGui::BeginChild("BoneTreeView", ImVec2(0, 0), true);
                    const TArray<FBone>& Bones = Skeleton->Bones;
                    TArray<TArray<int32>> Children;
                    Children.resize(Bones.size());
                    for (int32 i = 0; i < Bones.size(); ++i)
                    {
                        int32 Parent = Bones[i].ParentIndex;
                        if (Parent >= 0 && Parent < Bones.size())
                        {
                            Children[Parent].Add(i);
                        }
                    }

                    std::function<void(int32)> DrawNode = [&](int32 Index)
                    {
                        const bool bLeaf = Children[Index].IsEmpty();
                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
                        
                        if (bLeaf)
                        {
                            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                        }
                        
                        // 펼쳐진 노드는 명시적으로 열린 상태로 설정
                        if (ActiveState->ExpandedBoneIndices.count(Index) > 0)
                        {
                            ImGui::SetNextItemOpen(true);
                        }
                        
                        if (ActiveState->SelectedBoneIndex == Index)
                        {
                            flags |= ImGuiTreeNodeFlags_Selected;
                        }

                        ImGui::PushID(Index);
                        const char* Label = Bones[Index].Name.c_str();

                        if (ActiveState->SelectedBoneIndex == Index)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.55f, 0.85f, 0.8f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.40f, 0.60f, 0.90f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.50f, 0.80f, 1.0f));
                        }

                        bool open = ImGui::TreeNodeEx((void*)(intptr_t)Index, flags, "%s", Label ? Label : "<noname>");

                        if (ActiveState->SelectedBoneIndex == Index)
                        {
                            ImGui::PopStyleColor(3);
                            
                            // 선택된 본까지 스크롤
                            ImGui::SetScrollHereY(0.5f);
                        }

                        // 사용자가 수동으로 노드를 접거나 펼쳤을 때 상태 업데이트
                        if (ImGui::IsItemToggledOpen())
                        {
                            if (open)
                                ActiveState->ExpandedBoneIndices.insert(Index);
                            else
                                ActiveState->ExpandedBoneIndices.erase(Index);
                        }

                        if (ImGui::IsItemClicked())
                        {
                            if (ActiveState->SelectedBoneIndex != Index)
                            {
                                ActiveState->SelectedBoneIndex = Index;
                                ActiveState->bBoneLinesDirty = true;
                                
                                ExpandToSelectedBone(ActiveState, Index);

                                if (ActiveState->PreviewActor && ActiveState->World)
                                {
                                    ActiveState->PreviewActor->RepositionAnchorToBone(Index);
                                    if (USceneComponent* Anchor = ActiveState->PreviewActor->GetBoneGizmoAnchor())
                                    {
                                        ActiveState->World->GetSelectionManager()->SelectActor(ActiveState->PreviewActor);
                                        ActiveState->World->GetSelectionManager()->SelectComponent(Anchor);
                                    }
                                }
                            }
                        }
                        
                        if (!bLeaf && open)
                        {
                            for (int32 Child : Children[Index])
                            {
                                DrawNode(Child);
                            }
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    };

                    for (int32 i = 0; i < Bones.size(); ++i)
                    {
                        if (Bones[i].ParentIndex < 0)
                        {
                            DrawNode(i);
                        }
                    }

                    ImGui::EndChild();
                }
            }
        }
        else
        {
            ImGui::EndChild();
            ImGui::End();
            return;
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        // Center panel
        ImGui::BeginChild("SkeletalMeshViewport", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 childPos = ImGui::GetWindowPos();
        ImVec2 childSize = ImGui::GetWindowSize();
        ImVec2 rectMin = childPos;
        ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);
        CenterRect.Left = rectMin.x; CenterRect.Top = rectMin.y; CenterRect.Right = rectMax.x; CenterRect.Bottom = rectMax.y; CenterRect.UpdateMinMax();

        // 뷰포트 크기/라인 데이터를 최신 상태로 유지
        OnRenderViewport();

        // 렌더 타겟을 갱신하고 결과 텍스처를 ImGui에 표시
        if (ActiveState && ActiveState->Viewport)
        {
            RenderActiveViewportToTexture(static_cast<uint32>(std::max(1.0f, childSize.x)), static_cast<uint32>(std::max(1.0f, childSize.y)));
        }

        if (ActiveState && ActiveState->ViewerSRV && ActiveState->bViewportTextureValid)
        {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::Image(
                (void*)ActiveState->ViewerSRV,
                ImVec2(childSize.x, childSize.y),
                ImVec2(0, 0),
                ImVec2(1, 1));
        }
        else
        {
            ImGui::SetCursorPos(ImVec2(childSize.x * 0.5f - 60.0f, childSize.y * 0.5f - 10.0f));
            ImGui::TextUnformatted("Viewport 미사용");
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        // Right panel
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
        ImGui::PopStyleVar();

        // Panel header
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        ImGui::Indent(8.0f);
        ImGui::Text("Bone Properties");
        ImGui::Unindent(8.0f);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // === 선택된 본의 트랜스폼 편집 UI ===
        if (ActiveState->SelectedBoneIndex >= 0 && ActiveState->CurrentMesh)
        {
            const FSkeleton* Skeleton = ActiveState->CurrentMesh->GetSkeleton();
            if (Skeleton && ActiveState->SelectedBoneIndex < Skeleton->Bones.size())
            {
                const FBone& SelectedBone = Skeleton->Bones[ActiveState->SelectedBoneIndex];

                // Selected bone header with icon-like prefix
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.90f, 0.40f, 1.0f));
                ImGui::Text("> Selected Bone");
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.95f, 1.00f, 1.0f));
                ImGui::TextWrapped("%s", SelectedBone.Name.c_str());
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.45f, 0.55f, 0.70f, 0.8f));
                ImGui::Separator();
                ImGui::PopStyleColor();

                // 본의 현재 트랜스폼 가져오기 (편집 중이 아닐 때만)
                if (!ActiveState->bBoneRotationEditing)
                {
                    UpdateBoneTransformFromSkeleton(ActiveState);
                }

                ImGui::Spacing();

                // Location 편집
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("Location");
                ImGui::PopStyleColor();

                ImGui::PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.28f, 0.20f, 0.20f, 0.6f));
                bool bLocationChanged = false;
                bLocationChanged |= ImGui::DragFloat("##BoneLocX", &ActiveState->EditBoneLocation.X, 0.1f, 0.0f, 0.0f, "X: %.3f");
                bLocationChanged |= ImGui::DragFloat("##BoneLocY", &ActiveState->EditBoneLocation.Y, 0.1f, 0.0f, 0.0f, "Y: %.3f");
                bLocationChanged |= ImGui::DragFloat("##BoneLocZ", &ActiveState->EditBoneLocation.Z, 0.1f, 0.0f, 0.0f, "Z: %.3f");
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();

                if (bLocationChanged)
                {
                    ApplyBoneTransform(ActiveState);
                    ActiveState->bBoneLinesDirty = true;
                }

                ImGui::Spacing();

                // Rotation 편집
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
                ImGui::Text("Rotation");
                ImGui::PopStyleColor();

                ImGui::PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.28f, 0.20f, 0.6f));
                bool bRotationChanged = false;

                if (ImGui::IsAnyItemActive())
                {
                    ActiveState->bBoneRotationEditing = true;
                }

                bRotationChanged |= ImGui::DragFloat("##BoneRotX", &ActiveState->EditBoneRotation.X, 0.5f, -180.0f, 180.0f, "X: %.2f°");
                bRotationChanged |= ImGui::DragFloat("##BoneRotY", &ActiveState->EditBoneRotation.Y, 0.5f, -180.0f, 180.0f, "Y: %.2f°");
                bRotationChanged |= ImGui::DragFloat("##BoneRotZ", &ActiveState->EditBoneRotation.Z, 0.5f, -180.0f, 180.0f, "Z: %.2f°");
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();

                if (!ImGui::IsAnyItemActive())
                {
                    ActiveState->bBoneRotationEditing = false;
                }

                if (bRotationChanged)
                {
                    ApplyBoneTransform(ActiveState);
                    ActiveState->bBoneLinesDirty = true;
                }

                ImGui::Spacing();

                // Scale 편집
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 1.0f, 1.0f));
                ImGui::Text("Scale");
                ImGui::PopStyleColor();

                ImGui::PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.20f, 0.28f, 0.6f));
                bool bScaleChanged = false;
                bScaleChanged |= ImGui::DragFloat("##BoneScaleX", &ActiveState->EditBoneScale.X, 0.01f, 0.001f, 100.0f, "X: %.3f");
                bScaleChanged |= ImGui::DragFloat("##BoneScaleY", &ActiveState->EditBoneScale.Y, 0.01f, 0.001f, 100.0f, "Y: %.3f");
                bScaleChanged |= ImGui::DragFloat("##BoneScaleZ", &ActiveState->EditBoneScale.Z, 0.01f, 0.001f, 100.0f, "Z: %.3f");
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();

                if (bScaleChanged)
                {
                    ApplyBoneTransform(ActiveState);
                    ActiveState->bBoneLinesDirty = true;
                }
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("Select a bone from the hierarchy to edit its transform properties.");
            ImGui::PopStyleColor();
        }

        ImGui::EndChild(); // RightPanel

        // Pop the ItemSpacing style
        ImGui::PopStyleVar();

        // Render Animation Mode checkbox (always visible)
        RenderAnimationModeCheckbox();

        // Render the playback bar only if animation mode is enabled
        if (ActiveState && ActiveState->bAnimationMode)
        {
            RenderPlaybackBar(FinalPlaybackBarHeight);
        }
    }
    ImGui::End();

    // If collapsed or not visible, clear the center rect so we don't render a floating viewport
    if (!bViewerVisible)
    {
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
    }

    // If window was closed via X button, notify the manager to clean up
    if (!bIsOpen)
    {
        USlateManager::GetInstance().CloseSkeletalMeshViewer();
    }

    bRequestFocus = false;
}

void SSkeletalMeshViewerWindow::RenderAnimationModeCheckbox()
{
	if (!ActiveState || !ActiveState->PreviewActor)
		return;

	USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
	if (!SkeletalComp)
		return;

	// Animation Mode checkbox container
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
	ImGui::BeginChild("AnimationModeCheckbox", ImVec2(0, AnimationModeCheckboxHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleColor();

	// Center the checkbox vertically - calculate proper offset
	float availableHeight = ImGui::GetContentRegionAvail().y;
	float checkboxHeight = ImGui::GetFrameHeight();
	float verticalOffset = (availableHeight - checkboxHeight) * 0.5f;

	if (verticalOffset > 0.0f)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);
	}

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.30f, 0.35f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.40f, 0.70f, 1.00f, 1.0f));

	if (ImGui::Checkbox("Animation Mode", &ActiveState->bAnimationMode))
	{
		// 체크박스 토글 시 처리
		if (ActiveState->bAnimationMode)
		{
			// Animation Mode로 전환
			SkeletalComp->SetEnableAnimation(true);
			UE_LOG("Animation Mode enabled");
		}
		else
		{
			// Bone Edit Mode로 전환 - 애니메이션 중지
			UE_LOG("Animation Mode disabled - switching to Bone Edit Mode");
			ActiveState->bOnChangedToBoneMode = true;
			SkeletalComp->SetEnableAnimation(false);
		}
	}

	ImGui::PopStyleColor(2);

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Enable/Disable animation playback mode");
	}

	ImGui::EndChild();
}

void SSkeletalMeshViewerWindow::RenderPlaybackBar(float AvailableHeight)
{
    if (!ActiveState || !ActiveState->PreviewActor)
        return;

    USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
    if (!SkeletalComp)
        return;

    // Playback bar background - use the available height
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    ImGui::BeginChild("PlaybackBar", ImVec2(0, AvailableHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();

    // Get animation instance
    UAnimSingleNodeInstance* AnimInstance = SkeletalComp->GetSingleNodeInstance();
    UAnimSequence* AnimSequence = AnimInstance ? AnimInstance->GetCurrentAnimSequence() : nullptr;
    UAnimDataModel* AnimDataModel = AnimSequence ? AnimSequence->GetDataModel() : nullptr;
    bool bHasAnimationSequence = AnimInstance && AnimSequence && AnimDataModel;

    bool bIsPlaying = AnimInstance && AnimInstance->IsPlaying();

	static int32 GRenameTrackIndex = -1;
	static bool GRenameError = false;
	static bool GRenameShouldFocus = false;
	static char GRenameBuffer[64] = {0};
	static bool GShouldOpenAddNotifyPopup = false;
	static int32 GPendingAddNotifyTrackIndex = -1;
	static float GPendingAddNotifyTime = 0.f;

    // Playback controls section
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
    ImGui::Spacing();

    // Playback controls
    ImGui::BeginGroup();
    
    // Play button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.70f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.80f, 0.50f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.60f, 0.30f, 1.0f));
    
    if (!bHasAnimationSequence)
    {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("Play", ImVec2(80, 30)))
    {
        OnPlayButtonPressed();
    }
    
    if (!bHasAnimationSequence)
    {
        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Pause button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.60f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.70f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.50f, 0.15f, 1.0f));
    
    if (!bHasAnimationSequence || !bIsPlaying)
    {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("Pause", ImVec2(80, 30)))
    {
        OnPauseButtonPressed();
    }
    
    if (!bHasAnimationSequence || !bIsPlaying)
    {
        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Stop button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.30f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.15f, 0.15f, 1.0f));
    
    if (!bHasAnimationSequence)
    {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("Stop", ImVec2(80, 30)))
    {
        OnStopButtonPressed();
    }
    
    if (!bHasAnimationSequence)
    {
        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

	// Reverse Play button
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.20f, 0.70f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.30f, 0.80f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.15f, 0.60f, 1.0f));

	if (!bHasAnimationSequence)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Reverse", ImVec2(80, 30)))
	{
		OnReversePlayButtonPressed();
	}

	if (!bHasAnimationSequence)
	{
		ImGui::EndDisabled();
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();

    ImGui::Spacing();
    ImGui::SameLine();

    // Animation status text
    if (bHasAnimationSequence)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        const char* statusText = bIsPlaying ? "Playing" : "Stopped";
        ImGui::Text("Status: %s | Loop: %s", statusText, AnimInstance->IsLooping() ? "On" : "Off");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("No animation loaded");
        ImGui::PopStyleColor();
    }

    ImGui::EndGroup();

    // Timeline section (Unreal Engine style)
    if (bHasAnimationSequence)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float AnimLength = AnimDataModel->GetPlayLength();
        float CurrentTime = AnimInstance->GetCurrentTime();
        const FFrameRate& FrameRate = AnimDataModel->GetFrameRate();
        float FrameRateFloat = FrameRate.ToFloat();
        int32 TotalFrames = static_cast<int32>(AnimLength * FrameRateFloat);

        const TArray<FName>& NotifyTracks = AnimSequence->NotifyTracks;
        const int32 TrackCountRaw = NotifyTracks.Num();
        const int32 DisplayTrackCount = TrackCountRaw > 0 ? TrackCountRaw : 1;

        if (GRenameTrackIndex >= TrackCountRaw)
        {
            GRenameTrackIndex = -1;
            GRenameError = false;
            std::memset(GRenameBuffer, 0, sizeof(GRenameBuffer));
            GRenameShouldFocus = false;
        }

        if (GPendingAddNotifyTrackIndex >= TrackCountRaw)
        {
            GPendingAddNotifyTrackIndex = -1;
        }

        ImGui::BeginGroup();
        
        // Time display with frame info
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.9f, 1.0f));
        int32 CurrentFrame = static_cast<int32>(CurrentTime * FrameRateFloat);
        ImGui::Text("Time: %.2f / %.2f s | Frame: %d / %d (%.1f fps)", 
                    CurrentTime, AnimLength, CurrentFrame, TotalFrames, FrameRateFloat);
        ImGui::PopStyleColor();

        // Track add button aligned to the right
        const float TrackButtonWidth = 110.0f;
        float lineCursorX = ImGui::GetCursorPosX();
        float lineAvailableWidth = std::max(0.0f, ImGui::GetContentRegionAvail().x);
        ImGui::SameLine();
        ImGui::SetCursorPosX(lineCursorX + std::max(0.0f, lineAvailableWidth - TrackButtonWidth));
        const bool bIsTrackCapacityReached = TrackCountRaw >= UAnimSequenceBase::MaxNumNotifyTracks;
        if (bIsTrackCapacityReached)
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("+ Track", ImVec2(TrackButtonWidth, 0.0f)))
        {
            AnimSequence->AddNotifyTrack();
        }
        if (bIsTrackCapacityReached)
        {
            ImGui::EndDisabled();
        }

        ImGui::Spacing();

        const float PlayButtonWidth = 80.0f;
        const float TrackLabelColumnWidth = PlayButtonWidth * 2.0f;
        const float DesiredTrackHeight = 40.0f; // 각 트랙 고정 높이
        const float TrackLabelPadding = 8.0f;

        float totalWidth = ImGui::GetContentRegionAvail().x;
        float perTrackHeight = DesiredTrackHeight;
        float timelineHeight = perTrackHeight * static_cast<float>(DisplayTrackCount);

        float timelineWidth = std::max(1.0f, totalWidth - TrackLabelColumnWidth);

        float frameLabelHeight = ImGui::GetTextLineHeight();
        ImVec2 frameLabelTopPos = ImGui::GetCursorScreenPos();
        ImVec2 timelinePos = ImVec2(frameLabelTopPos.x, frameLabelTopPos.y + frameLabelHeight + 4.0f);
        ImVec2 trackAreaOrigin = ImVec2(timelinePos.x + TrackLabelColumnWidth, timelinePos.y);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        bool bHasValidFrames = (TotalFrames > 0 && AnimLength > 0.0f);
        int32 frameInterval = 1;
        if (bHasValidFrames)
        {
            if (TotalFrames > 200) frameInterval = 10;
            else if (TotalFrames > 100) frameInterval = 5;
            else if (TotalFrames > 50) frameInterval = 2;
        }

        // Frame labels (placed above the first timeline row)
        if (bHasValidFrames)
        {
            for (int32 Frame = 0; Frame <= TotalFrames; ++Frame)
            {
                float normalizedPos = static_cast<float>(Frame) / static_cast<float>(TotalFrames);
                float lineX = trackAreaOrigin.x + normalizedPos * timelineWidth;

                bool bIsMajorTick = (Frame % (frameInterval * 5) == 0);

                if (bIsMajorTick || Frame == 0 || Frame == TotalFrames)
                {
                    char frameLabel[16];
                    sprintf_s(frameLabel, "%d", Frame);
                    ImVec2 textSize = ImGui::CalcTextSize(frameLabel);
                    drawList->AddText(
                        ImVec2(lineX - textSize.x * 0.5f, timelinePos.y - textSize.y - 4.0f),
                        IM_COL32(180, 190, 200, 255),
                        frameLabel
                    );
                }
            }
        }

        float playheadRatio = (AnimLength > 0.0f) ? (CurrentTime / AnimLength) : 0.0f;
        float playheadX = trackAreaOrigin.x + playheadRatio * timelineWidth;
        const float triangleSize = 8.0f;

        bool bAnyTimelineActive = false;

        for (int32 TrackIdx = 0; TrackIdx < DisplayTrackCount; ++TrackIdx)
        {
            float rowTop = timelinePos.y + TrackIdx * perTrackHeight;
            ImVec2 labelMin(timelinePos.x, rowTop);
            ImVec2 labelMax(timelinePos.x + TrackLabelColumnWidth, rowTop + perTrackHeight);
            ImVec2 laneMin(trackAreaOrigin.x, rowTop);
            ImVec2 laneMax(trackAreaOrigin.x + timelineWidth, rowTop + perTrackHeight);

            ImU32 labelColor = (TrackIdx % 2 == 0) ? IM_COL32(24, 24, 30, 255) : IM_COL32(28, 28, 34, 255);
            ImU32 laneColor = (TrackIdx % 2 == 0) ? IM_COL32(27, 27, 33, 255) : IM_COL32(31, 31, 38, 255);

            drawList->AddRectFilled(labelMin, labelMax, labelColor, 4.0f);
            drawList->AddRectFilled(laneMin, laneMax, laneColor, 4.0f);
            drawList->AddRect(laneMin, laneMax, IM_COL32(60, 70, 80, 255), 4.0f, 0, 1.1f);

            // Track label interactions
            ImGui::SetCursorScreenPos(labelMin);
            ImGui::PushID(TrackIdx);
            ImGui::InvisibleButton("NotifyTrackLabel", ImVec2(TrackLabelColumnWidth, perTrackHeight));
            bool bLabelHovered = ImGui::IsItemHovered();
            bool bLabelDoubleClicked = bLabelHovered && ImGui::IsMouseDoubleClicked(0);
            if (ImGui::BeginPopupContextItem("NotifyTrackContext"))
            {
                const bool bHasRealTrack = TrackIdx < TrackCountRaw;

                if (ImGui::MenuItem("Delete Track"))
                {
                    if (bHasRealTrack)
                    {
                        AnimSequence->DeleteNotifyTrack(TrackIdx);
                    }
                }

                ImGui::EndPopup();
            }
            if (bLabelDoubleClicked && TrackIdx < TrackCountRaw)
            {
                GRenameTrackIndex = TrackIdx;
                const FString& CurrentName = NotifyTracks[TrackIdx].ToString();
                std::memset(GRenameBuffer, 0, sizeof(GRenameBuffer));
                strncpy_s(GRenameBuffer, sizeof(GRenameBuffer), CurrentName.c_str(), _TRUNCATE);
                GRenameError = false;
                GRenameShouldFocus = true;
            }
            ImGui::PopID();

            FString TrackLabel;
            if (TrackIdx < TrackCountRaw && TrackCountRaw > 0)
            {
                TrackLabel = NotifyTracks[TrackIdx].ToString();
            }
            else
            {
                TrackLabel = FString("Track ") + std::to_string(TrackIdx);
            }

            bool bIsEditingTrack = (GRenameTrackIndex == TrackIdx);
            if (bIsEditingTrack)
            {
                ImVec2 inputPos = ImVec2(labelMin.x + TrackLabelPadding, rowTop + (perTrackHeight * 0.5f) - (ImGui::GetFrameHeight() * 0.5f));
                ImGui::SetCursorScreenPos(inputPos);
                ImGui::PushItemWidth(TrackLabelColumnWidth - TrackLabelPadding * 2.0f);
                ImGuiInputTextFlags InputFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
                if (GRenameShouldFocus)
                {
                    ImGui::SetKeyboardFocusHere();
                    GRenameShouldFocus = false;
                }
                bool bInputConfirmed = ImGui::InputText("##TrackRenameField", GRenameBuffer, sizeof(GRenameBuffer), InputFlags);
                ImGui::PopItemWidth();

                bool bItemDeactivated = ImGui::IsItemDeactivated();
                bool bItemDeactivatedAfterEdit = ImGui::IsItemDeactivatedAfterEdit();
                bool bShouldCommit = bInputConfirmed || bItemDeactivatedAfterEdit;
                bool bShouldCancel = false;

                if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    bShouldCancel = true;
                }
                else if (bItemDeactivated && !bItemDeactivatedAfterEdit && !bInputConfirmed)
                {
                    bShouldCancel = true;
                }

                auto CommitRename = [&]()
                {
                    FString NewName = GRenameBuffer;
                    if (!NewName.empty() && GRenameTrackIndex >= 0 && GRenameTrackIndex < TrackCountRaw)
                    {
                        if (AnimSequence->RenameNotifyTrack(GRenameTrackIndex, FName(NewName)))
                        {
                            GRenameTrackIndex = -1;
                            GRenameError = false;
                            GRenameShouldFocus = false;
                            std::memset(GRenameBuffer, 0, sizeof(GRenameBuffer));
                        }
                        else
                        {
                            GRenameError = true;
                        }
                    }
                    else
                    {
                        GRenameError = true;
                    }
                };

                auto CancelRename = [&]()
                {
                    GRenameTrackIndex = -1;
                    GRenameError = false;
                    GRenameShouldFocus = false;
                    std::memset(GRenameBuffer, 0, sizeof(GRenameBuffer));
                };

                if (bShouldCommit)
                {
                    CommitRename();
                }
                else if (bShouldCancel)
                {
                    CancelRename();
                }

                if (GRenameError)
                {
                    drawList->AddText(
                        ImVec2(labelMin.x + TrackLabelPadding, labelMax.y - ImGui::GetTextLineHeight() - 2.0f),
                        IM_COL32(255, 100, 100, 255),
                        "중복된 이름입니다."
                    );
                }
            }
            else
            {
                ImVec2 textSize = ImGui::CalcTextSize(TrackLabel.c_str());
                float textY = rowTop + (perTrackHeight - textSize.y) * 0.5f;
                drawList->AddText(
                    ImVec2(labelMin.x + TrackLabelPadding, textY),
                    IM_COL32(210, 210, 230, 255),
                    TrackLabel.c_str()
                );
            }

            if (bHasValidFrames)
            {
                for (int32 Frame = 0; Frame <= TotalFrames; ++Frame)
                {
                    float normalizedPos = static_cast<float>(Frame) / static_cast<float>(TotalFrames);
                    float lineX = laneMin.x + normalizedPos * timelineWidth;

                    bool bIsMajorTick = (Frame % (frameInterval * 5) == 0);
                    bool bIsMinorTick = (Frame % frameInterval == 0);

                    ImU32 lineColor;
                    float tickPadding = 4.0f;
                    float tickStart = laneMin.y + tickPadding;
                    float tickEnd = laneMax.y - tickPadding;

                    if (bIsMajorTick || Frame == 0 || Frame == TotalFrames)
                    {
                        lineColor = IM_COL32(140, 160, 180, 200);
                    }
                    else if (bIsMinorTick)
                    {
                        lineColor = IM_COL32(80, 90, 100, 150);
                        float shrink = (tickEnd - tickStart) * 0.25f;
                        tickStart += shrink;
                        tickEnd -= shrink;
                    }
                    else
                    {
                        lineColor = IM_COL32(50, 55, 60, 100);
                        float shrink = (tickEnd - tickStart) * 0.4f;
                        tickStart += shrink;
                        tickEnd -= shrink;
                    }

                    drawList->AddLine(
                        ImVec2(lineX, tickStart),
                        ImVec2(lineX, tickEnd),
                        lineColor,
                        bIsMajorTick ? 1.5f : 1.0f
                    );
                }
            }

            // Playhead per track
            drawList->AddLine(
                ImVec2(playheadX, laneMin.y),
                ImVec2(playheadX, laneMax.y),
                IM_COL32(255, 80, 80, 255),
                3.0f
            );

            // Interaction per track lane
            ImGui::SetCursorScreenPos(laneMin);
            ImGui::PushID(TrackIdx + 100);
            ImGui::InvisibleButton("TimelineScrubberTrack", ImVec2(timelineWidth, perTrackHeight));
            bool bLaneHovered = ImGui::IsItemHovered();
            bool bLaneActive = ImGui::IsItemActive();
            bool bLaneRightClicked = bLaneHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

            if (bLaneRightClicked)
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                float normalizedPos = (mousePos.x - laneMin.x) / timelineWidth;
                normalizedPos = std::clamp(normalizedPos, 0.0f, 1.0f);

                GPendingAddNotifyTrackIndex = TrackIdx;
                GPendingAddNotifyTime = normalizedPos * AnimLength;

                ImGui::OpenPopup("TrackLaneContextMenu");
            }

            if (ImGui::BeginPopup("TrackLaneContextMenu"))
            {
                if (ImGui::MenuItem("Add Notify"))
                {
                    GShouldOpenAddNotifyPopup = true;
                }

                ImGui::EndPopup();
            }

            if (bLaneActive || (bLaneHovered && ImGui::IsMouseClicked(0)))
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                float normalizedPos = (mousePos.x - laneMin.x) / timelineWidth;
                normalizedPos = std::max(0.0f, std::min(1.0f, normalizedPos));

                float newTime = normalizedPos * AnimLength;

                if (!ActiveState->bTimelineScrubbing)
                {
                    ActiveState->bTimelineScrubbing = true;
                    ActiveState->bWasPlayingBeforeScrub = bIsPlaying;
                    if (bIsPlaying)
                    {
                        AnimInstance->SetPlaying(false);
                    }
                }

                AnimInstance->SetCurrentTime(newTime);
                SkeletalComp->ForceRecomputePose();
                ActiveState->bBoneLinesDirty = true;
            }

            if (bLaneActive)
            {
                bAnyTimelineActive = true;
            }

            ImGui::PopID();
        }

        if (ActiveState->bTimelineScrubbing && !bAnyTimelineActive)
        {
            ActiveState->bTimelineScrubbing = false;
            if (ActiveState->bWasPlayingBeforeScrub)
            {
                AnimInstance->SetPlaying(true);
            }
        }

        if (GShouldOpenAddNotifyPopup)
        {
            ImGui::OpenPopup("AddAnimNotifyPopup");
            GShouldOpenAddNotifyPopup = false;
        }

        auto ResetPendingAddNotifyState = [&]()
        {
            GPendingAddNotifyTrackIndex = -1;
            GPendingAddNotifyTime = 0.f;
        };

        if (ImGui::BeginPopupModal("AddAnimNotifyPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("추가할 AnimNotify 클래스를 선택하세요.");
            if (GPendingAddNotifyTrackIndex >= 0)
            {
                ImGui::Text("Track: %d | Time: %.3f s", GPendingAddNotifyTrackIndex, GPendingAddNotifyTime);
            }
            ImGui::Separator();

            const TArray<UClass*>& NotifyClasses = GetAnimNotifyClassList();
            if (NotifyClasses.IsEmpty())
            {
                ImGui::Text("등록된 AnimNotify 파생 클래스가 없습니다.");
            }
            else
            {
                for (UClass* NotifyClass : NotifyClasses)
                {
                    if (!NotifyClass)
                    {
                        continue;
                    }

                    const char* Label = (NotifyClass->DisplayName && NotifyClass->DisplayName[0] != '\0')
                        ? NotifyClass->DisplayName
                        : NotifyClass->Name;

                    if (ImGui::Selectable(Label, false))
                    {
                        UE_LOG("Selected AnimNotify class: %s", Label ? Label : "Unknown");
                        ResetPendingAddNotifyState();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            if (ImGui::Button("닫기", ImVec2(80.0f, 0.0f)))
            {
                ResetPendingAddNotifyState();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // Playhead triangle indicator
        drawList->AddTriangleFilled(
            ImVec2(playheadX, timelinePos.y - 2.0f),
            ImVec2(playheadX - triangleSize, timelinePos.y - 2.0f - triangleSize),
            ImVec2(playheadX + triangleSize, timelinePos.y - 2.0f - triangleSize),
            IM_COL32(255, 80, 80, 255)
        );

        // Border around the entire area for clarity
        drawList->AddRect(
            timelinePos,
            ImVec2(timelinePos.x + totalWidth, timelinePos.y + timelineHeight),
            IM_COL32(60, 70, 80, 255),
            0.0f,
            0,
            1.5f
        );

        // Advance the cursor past labels + timeline + spacing reserved for frame numbers
        ImGui::SetCursorScreenPos(frameLabelTopPos);
        ImGui::Dummy(ImVec2(totalWidth, (frameLabelHeight + 4.0f) + timelineHeight));

        ImGui::EndGroup();
    }

    ImGui::EndChild();
}

void SSkeletalMeshViewerWindow::OnPlayButtonPressed()
{
    if (!ActiveState || !ActiveState->PreviewActor)
        return;

    USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
    if (!SkeletalComp)
        return;

	SkeletalComp->PlayDefaultAnimation();
	ActiveState->bAnimationMode = true;
}

void SSkeletalMeshViewerWindow::OnPauseButtonPressed()
{
    if (!ActiveState || !ActiveState->PreviewActor)
        return;

    USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
    if (!SkeletalComp)
        return;

    UAnimSingleNodeInstance* AnimInstance = SkeletalComp->GetSingleNodeInstance();
    if (AnimInstance)
    {
        AnimInstance->SetPlaying(false);
    }
}

void SSkeletalMeshViewerWindow::OnStopButtonPressed()
{
    if (!ActiveState || !ActiveState->PreviewActor)
        return;

    USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
    if (!SkeletalComp)
        return;

	SkeletalComp->Stop();
}

void SSkeletalMeshViewerWindow::OnReversePlayButtonPressed()
{
	if (!ActiveState || !ActiveState->PreviewActor)
		return;

	USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
	if (!SkeletalComp)
		return;

	UAnimSingleNodeInstance* AnimInstance = SkeletalComp->GetSingleNodeInstance();
	if (!AnimInstance)
		return;

	// Get current animation sequence
	UAnimSequence* AnimSequence = AnimInstance->GetCurrentAnimSequence();
	if (!AnimSequence)
		return;

	// Set negative play rate for reverse playback
	AnimInstance->SetPlayRate(-1.0f);
	AnimInstance->SetPlaying(true);

	// If at the beginning, jump to the end to play in reverse
	if (AnimInstance->GetCurrentTime() <= 0.0f)
	{
		UAnimDataModel* DataModel = AnimSequence->GetDataModel();
		if (DataModel)
		{
			AnimInstance->SetCurrentTime(DataModel->GetPlayLength(), true);
		}
	}

	ActiveState->bAnimationMode = true;
	UE_LOG("Reverse playback started");
}

void SSkeletalMeshViewerWindow::OnUpdate(float DeltaSeconds)
{
    if (!ActiveState || !ActiveState->Viewport)
        return;

    // Tick the preview world so editor actors (e.g., gizmo) update visibility/state
    if (ActiveState->World)
    {
        ActiveState->World->Tick(DeltaSeconds);
        if (ActiveState->World->GetGizmoActor())
            ActiveState->World->GetGizmoActor()->ProcessGizmoModeSwitch();
    }

    if (ActiveState && ActiveState->Client)
    {
        ActiveState->Client->Tick(DeltaSeconds);
    }
}

void SSkeletalMeshViewerWindow::OnMouseMove(FVector2D MousePos)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        ActiveState->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SSkeletalMeshViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);

        // First, always try gizmo picking (pass to viewport)
        ActiveState->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);

        // Left click: if no gizmo was picked, try bone picking
        if (Button == 0 && ActiveState->PreviewActor && ActiveState->CurrentMesh && ActiveState->Client && ActiveState->World)
        {
            // Check if gizmo was picked by checking selection
            UActorComponent* SelectedComp = ActiveState->World->GetSelectionManager()->GetSelectedComponent();

            // Only do bone picking if gizmo wasn't selected
            if (!SelectedComp || !Cast<UBoneAnchorComponent>(SelectedComp))
            {
                // Get camera from viewport client
                ACameraActor* Camera = ActiveState->Client->GetCamera();
                if (Camera)
                {
                    // Get camera vectors
                    FVector CameraPos = Camera->GetActorLocation();
                    FVector CameraRight = Camera->GetRight();
                    FVector CameraUp = Camera->GetUp();
                    FVector CameraForward = Camera->GetForward();

                    // Calculate viewport-relative mouse position
                    FVector2D ViewportMousePos(MousePos.X - CenterRect.Left, MousePos.Y - CenterRect.Top);
                    FVector2D ViewportSize(CenterRect.GetWidth(), CenterRect.GetHeight());

                    // Generate ray from mouse position
                    FRay Ray = MakeRayFromViewport(
                        Camera->GetViewMatrix(),
                        Camera->GetProjectionMatrix(CenterRect.GetWidth() / CenterRect.GetHeight(), ActiveState->Viewport),
                        CameraPos,
                        CameraRight,
                        CameraUp,
                        CameraForward,
                        ViewportMousePos,
                        ViewportSize
                    );

                    // Try to pick a bone
                    float HitDistance;
                    int32 PickedBoneIndex = ActiveState->PreviewActor->PickBone(Ray, HitDistance);

                    if (PickedBoneIndex >= 0)
                    {
                        // Bone was picked
                        ActiveState->SelectedBoneIndex = PickedBoneIndex;
                        ActiveState->bBoneLinesDirty = true;

                        ExpandToSelectedBone(ActiveState, PickedBoneIndex);

                        // Move gizmo to the selected bone
                        ActiveState->PreviewActor->RepositionAnchorToBone(PickedBoneIndex);
                        if (USceneComponent* Anchor = ActiveState->PreviewActor->GetBoneGizmoAnchor())
                        {
                            ActiveState->World->GetSelectionManager()->SelectActor(ActiveState->PreviewActor);
                            ActiveState->World->GetSelectionManager()->SelectComponent(Anchor);
                        }
                    }
                    else
                    {
                        // No bone was picked - clear selection
                        ActiveState->SelectedBoneIndex = -1;
                        ActiveState->bBoneLinesDirty = true;

                        // Hide gizmo and clear selection
                        if (UBoneAnchorComponent* Anchor = ActiveState->PreviewActor->GetBoneGizmoAnchor())
                        {
                            Anchor->SetVisibility(false);
                            Anchor->SetEditability(false);
                        }
                        ActiveState->World->GetSelectionManager()->ClearSelection();
                    }
                }
            }
        }
    }
}

void SSkeletalMeshViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!ActiveState || !ActiveState->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        ActiveState->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SSkeletalMeshViewerWindow::OnRenderViewport()
{
    if (ActiveState && ActiveState->Viewport && CenterRect.GetWidth() > 0 && CenterRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(CenterRect.Left);
        const uint32 NewStartY = static_cast<uint32>(CenterRect.Top);
        const uint32 NewWidth  = static_cast<uint32>(CenterRect.Right - CenterRect.Left);
        const uint32 NewHeight = static_cast<uint32>(CenterRect.Bottom - CenterRect.Top);
        ActiveState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        // 본 오버레이 재구축
        if (ActiveState->bShowBones)
        {
            ActiveState->bBoneLinesDirty = true;
        }
        if (ActiveState->bShowBones && ActiveState->PreviewActor && ActiveState->CurrentMesh && ActiveState->bBoneLinesDirty)
        {
            if (ULineComponent* LineComp = ActiveState->PreviewActor->GetBoneLineComponent())
            {
                LineComp->SetLineVisible(true);
            }

			if (ActiveState->bAnimationMode)
			{
				ActiveState->PreviewActor->RebuildBoneLines(0); // 전체 본 갱신
			}
			else if (ActiveState->bOnChangedToBoneMode)
			{
				ActiveState->PreviewActor->RebuildBoneLines(0); // 전체 본 갱신
				ActiveState->bOnChangedToBoneMode = false;
			}
			else
			{
				ActiveState->PreviewActor->RebuildBoneLines(ActiveState->SelectedBoneIndex);
			}
			ActiveState->bBoneLinesDirty = false;
        }

        // 실제 렌더링은 ImGui 패널 안에서 텍스처를 갱신할 때 수행된다
    }
}

bool SSkeletalMeshViewerWindow::EnsureRenderTarget(ViewerState* State, uint32 Width, uint32 Height)
{
    if (!State || !Device || Width == 0 || Height == 0)
    {
        return false;
    }

    if (State->ViewerTexture &&
        State->ViewerTextureWidth == Width &&
        State->ViewerTextureHeight == Height)
    {
        return true;
    }

    ReleaseRenderTarget(State);

    D3D11_TEXTURE2D_DESC Desc = {};
    Desc.Width = Width;
    Desc.Height = Height;
    Desc.MipLevels = 1;
    Desc.ArraySize = 1;
    Desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    Desc.SampleDesc.Count = 1;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = Device->CreateTexture2D(&Desc, nullptr, &State->ViewerTexture);
    if (FAILED(hr))
    {
        UE_LOG("SSkeletalMeshViewerWindow: 렌더 타겟 생성 실패 (%u x %u)", Width, Height);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = Desc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(State->ViewerTexture, &SRVDesc, &State->ViewerSRV);
    if (FAILED(hr))
    {
        State->ViewerTexture->Release();
        State->ViewerTexture = nullptr;
        UE_LOG("SSkeletalMeshViewerWindow: 렌더 타겟 SRV 생성 실패");
        return false;
    }

    State->ViewerTextureWidth = Width;
    State->ViewerTextureHeight = Height;
    return true;
}

void SSkeletalMeshViewerWindow::ReleaseRenderTarget(ViewerState* State)
{
    if (!State) return;

    if (State->ViewerSRV)
    {
        State->ViewerSRV->Release();
        State->ViewerSRV = nullptr;
    }

    if (State->ViewerTexture)
    {
        State->ViewerTexture->Release();
        State->ViewerTexture = nullptr;
    }

    State->ViewerTextureWidth = 0;
    State->ViewerTextureHeight = 0;
    State->bViewportTextureValid = false;
}

void SSkeletalMeshViewerWindow::RenderActiveViewportToTexture(uint32 Width, uint32 Height)
{
    if (!ActiveState || !ActiveState->Viewport || Width == 0 || Height == 0)
    {
        if (ActiveState)
        {
            ActiveState->bViewportTextureValid = false;
        }
        return;
    }

    if (!EnsureRenderTarget(ActiveState, Width, Height))
    {
        ActiveState->bViewportTextureValid = false;
        return;
    }

    FViewportRenderOptions RenderOptions;
    RenderOptions.bCompositeToBackBuffer = false;
    RenderOptions.bCaptureSceneColor = true;
    RenderOptions.CaptureTexture = ActiveState->ViewerTexture;

    ActiveState->Viewport->SetRenderOptions(RenderOptions);
    ActiveState->Viewport->Render();
    ActiveState->Viewport->ResetRenderOptions();
    ActiveState->bViewportTextureValid = true;
}

void SSkeletalMeshViewerWindow::OpenNewTab(const char* Name)
{
    ViewerState* State = SkeletalViewerBootstrap::CreateViewerState(Name, World, Device);
    if (!State) return;

    Tabs.Add(State);
    ActiveTabIndex = Tabs.Num() - 1;
    ActiveState = State;
}

void SSkeletalMeshViewerWindow::CloseTab(int Index)
{
    if (Index < 0 || Index >= Tabs.Num()) return;
    ViewerState* State = Tabs[Index];
    ReleaseRenderTarget(State);
    SkeletalViewerBootstrap::DestroyViewerState(State);
    Tabs.RemoveAt(Index);
    if (Tabs.Num() == 0) { ActiveTabIndex = -1; ActiveState = nullptr; }
    else { ActiveTabIndex = std::min(Index, Tabs.Num() - 1); ActiveState = Tabs[ActiveTabIndex]; }
}

void SSkeletalMeshViewerWindow::LoadSkeletalMesh(const FString& Path)
{
    if (!ActiveState || Path.empty())
        return;

    // Load the skeletal mesh using the resource manager
    USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
    if (Mesh && ActiveState->PreviewActor)
    {
        // Set the mesh on the preview actor
        ActiveState->PreviewActor->SetSkeletalMesh(Path);
        ActiveState->CurrentMesh = Mesh;
        ActiveState->LoadedMeshPath = Path;  // Track for resource unloading

        // Update mesh path buffer for display in UI
        strncpy_s(ActiveState->MeshPathBuffer, Path.c_str(), sizeof(ActiveState->MeshPathBuffer) - 1);

		// Sync mesh visibility with checkbox state and, if in animation mode, enable animation
        if (auto* Skeletal = ActiveState->PreviewActor->GetSkeletalMeshComponent())
        {
            Skeletal->SetVisibility(ActiveState->bShowMesh);
        }

        // Mark bone lines as dirty to rebuild on next frame
        ActiveState->bBoneLinesDirty = true;

        // Clear and sync bone line visibility
        if (auto* LineComp = ActiveState->PreviewActor->GetBoneLineComponent())
        {
            LineComp->ClearLines();
            LineComp->SetLineVisible(ActiveState->bShowBones);
        }

        UE_LOG("SSkeletalMeshViewerWindow: Loaded skeletal mesh from %s", Path.c_str());
    }
    else
    {
        UE_LOG("SSkeletalMeshViewerWindow: Failed to load skeletal mesh from %s", Path.c_str());
    }
}

void SSkeletalMeshViewerWindow::LoadAnimation(const FString& Path)
{
	if (!ActiveState || Path.empty())
		return;

	if (ActiveState->PreviewActor)
	{
		USkeletalMeshComponent* SkeletalComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
		if (SkeletalComp)
		{
			SkeletalComp->PlayAnimationByFileName(Path, true);

			ActiveState->bAnimationMode = false;
			SkeletalComp->SetEnableAnimation(ActiveState->bAnimationMode);

			// Update animation path buffer for display in UI
			strncpy_s(ActiveState->AnimationPathBuffer, Path.c_str(), sizeof(ActiveState->AnimationPathBuffer) - 1);
			UE_LOG("SSkeletalMeshViewerWindow: Loaded animation from %s", Path.c_str());
		}
	}
	else
	{
		UE_LOG("SSkeletalMeshViewerWindow: Failed to load animation from %s", Path.c_str());
	}
}

void SSkeletalMeshViewerWindow::UpdateBoneTransformFromSkeleton(ViewerState* State)
{
    if (!State || !State->CurrentMesh || State->SelectedBoneIndex < 0)
        return;
        
    // 본의 로컬 트랜스폼에서 값 추출
    const FTransform& BoneTransform = State->PreviewActor->GetSkeletalMeshComponent()->GetBoneLocalTransform(State->SelectedBoneIndex);
    State->EditBoneLocation = BoneTransform.Translation;
    State->EditBoneRotation = BoneTransform.Rotation.ToEulerZYXDeg();
    State->EditBoneScale = BoneTransform.Scale3D;
}

void SSkeletalMeshViewerWindow::ApplyBoneTransform(ViewerState* State)
{
    if (!State || !State->CurrentMesh || State->SelectedBoneIndex < 0)
        return;

    FTransform NewTransform(State->EditBoneLocation, FQuat::MakeFromEulerZYX(State->EditBoneRotation), State->EditBoneScale);
    State->PreviewActor->GetSkeletalMeshComponent()->SetBoneLocalTransform(State->SelectedBoneIndex, NewTransform);
}

void SSkeletalMeshViewerWindow::ExpandToSelectedBone(ViewerState* State, int32 BoneIndex)
{
    if (!State || !State->CurrentMesh)
        return;
        
    const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
    if (!Skeleton || BoneIndex < 0 || BoneIndex >= Skeleton->Bones.size())
        return;
    
    // 선택된 본부터 루트까지 모든 부모를 펼침
    int32 CurrentIndex = BoneIndex;
    while (CurrentIndex >= 0)
    {
        State->ExpandedBoneIndices.insert(CurrentIndex);
        CurrentIndex = Skeleton->Bones[CurrentIndex].ParentIndex;
    }
}
