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

//void SSkeletalMeshViewerWindow::ApplyModernStyle()
//{
//    ImGuiStyle& style = ImGui::GetStyle();
//    ImVec4* colors = style.Colors;
//
//    // Modern dark theme colors
//    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
//    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
//    colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.14f, 0.15f, 0.95f);
//    colors[ImGuiCol_ChildBg]                = ImVec4(0.16f, 0.17f, 0.18f, 0.95f);
//    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.94f);
//    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
//    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
//    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
//    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.28f, 0.56f, 0.90f, 0.40f);
//    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.56f, 0.90f, 0.67f);
//    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
//    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
//    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.10f, 0.10f, 0.11f, 0.51f);
//    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
//    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
//    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
//    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
//    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
//    colors[ImGuiCol_CheckMark]              = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
//    colors[ImGuiCol_SliderGrab]             = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
//    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.37f, 0.61f, 0.93f, 1.00f);
//    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
//    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
//    colors[ImGuiCol_ButtonActive]           = ImVec4(0.26f, 0.54f, 0.86f, 1.00f);
//    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.25f, 0.30f, 0.55f);
//    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
//    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
//    colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.29f, 0.50f);
//    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.28f, 0.56f, 0.90f, 0.78f);
//    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
//    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.56f, 0.90f, 0.20f);
//    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.28f, 0.56f, 0.90f, 0.67f);
//    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.28f, 0.56f, 0.90f, 0.95f);
//    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.20f, 0.25f, 0.86f);
//    colors[ImGuiCol_TabHovered]             = ImVec4(0.28f, 0.56f, 0.90f, 0.80f);
//    colors[ImGuiCol_TabActive]              = ImVec4(0.23f, 0.40f, 0.63f, 1.00f);
//    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
//    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
//    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
//    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
//    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
//    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
//    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
//    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
//    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
//    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
//    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
//    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.28f, 0.56f, 0.90f, 0.35f);
//    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
//    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
//    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
//    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
//    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
//
//    // Rounded corners and padding
//    style.WindowPadding      = ImVec2(12, 12);
//    style.FramePadding       = ImVec2(8, 4);
//    style.ItemSpacing        = ImVec2(8, 6);
//    style.ItemInnerSpacing   = ImVec2(6, 4);
//    style.IndentSpacing      = 22.0f;
//    style.ScrollbarSize      = 16.0f;
//    style.GrabMinSize        = 8.0f;
//
//    // Rounded corners
//    style.WindowRounding     = 6.0f;
//    style.ChildRounding      = 6.0f;
//    style.FrameRounding      = 4.0f;
//    style.PopupRounding      = 4.0f;
//    style.ScrollbarRounding  = 9.0f;
//    style.GrabRounding       = 3.0f;
//    style.TabRounding        = 4.0f;
//
//    // Borders
//    style.WindowBorderSize   = 1.0f;
//    style.ChildBorderSize    = 1.0f;
//    style.PopupBorderSize    = 1.0f;
//    style.FrameBorderSize    = 0.0f;
//    style.TabBorderSize      = 0.0f;
//}

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
                if (ImGui::BeginTabItem(State->Name.c_str(), &open))
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
        float totalHeight = contentAvail.y;

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
                        }

                        if (ImGui::IsItemClicked())
                        {
                            if (ActiveState->SelectedBoneIndex != Index)
                            {
                                ActiveState->SelectedBoneIndex = Index;
                                ActiveState->bBoneLinesDirty = true;

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

        ImGui::SameLine(0, 0); // No spacing between panels

        // Center panel (viewport area) — draw with border to see the viewport area
        ImGui::BeginChild("SkeletalMeshViewport", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 childPos = ImGui::GetWindowPos();
        ImVec2 childSize = ImGui::GetWindowSize();
        ImVec2 rectMin = childPos;
        ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);
        CenterRect.Left = rectMin.x; CenterRect.Top = rectMin.y; CenterRect.Right = rectMax.x; CenterRect.Bottom = rectMax.y; CenterRect.UpdateMinMax();
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // Right panel - Bone Properties
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
            ActiveState->PreviewActor->RebuildBoneLines(ActiveState->SelectedBoneIndex);
            ActiveState->bBoneLinesDirty = false;
        }

        // 뷰포트 렌더링 (ImGui보다 먼저)
        ActiveState->Viewport->Render();
    }
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

        // Update mesh path buffer for display in UI
        strncpy_s(ActiveState->MeshPathBuffer, Path.c_str(), sizeof(ActiveState->MeshPathBuffer) - 1);

        // Sync mesh visibility with checkbox state
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
