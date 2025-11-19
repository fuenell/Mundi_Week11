#pragma once
#include <d3d11.h>

class UWorld; class FViewport; class FViewportClient; class ASkeletalMeshActor; class USkeletalMesh; class UAnimNotify;

class ViewerState
{
public:
    FName Name;
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;
    
    // Have a pointer to the currently selected mesh to render in the viewer
    ASkeletalMeshActor* PreviewActor = nullptr;
    USkeletalMesh* CurrentMesh = nullptr;
    FString LoadedMeshPath;  // Track loaded mesh path for unloading
    int32 SelectedBoneIndex = -1;
    bool bShowMesh = true;
    bool bShowBones = true;
    // Bone line rebuild control
    bool bBoneLinesDirty = true;      // true면 본 라인 재구성
    int32 LastSelectedBoneIndex = -1; // 색상 갱신을 위한 이전 선택 인덱스
    // UI path buffer per-tab
    char MeshPathBuffer[260] = {0};
	char AnimationPathBuffer[260] = { 0 };
    std::set<int32> ExpandedBoneIndices;

    // 본 트랜스폼 편집 관련
    FVector EditBoneLocation;
    FVector EditBoneRotation;  // Euler angles in degrees
    FVector EditBoneScale;
    
    bool bBoneTransformChanged = false;
    bool bBoneRotationEditing = false;

	// 애니메이션 관련
	bool bAnimationMode = false; // false면 본 편집 모드, true면 애니메이션 재생 모드
	bool bOnChangedToBoneMode = false; // bAnimationMode가 false로 변경됐는지 (bAnimationMode의 false에 대한 처리 완료 시, 바로 false로 변경)

    // Animation timeline scrubbing state
    bool bTimelineScrubbing = false;
    bool bWasPlayingBeforeScrub = false;

    // Offscreen 렌더 타겟 (ImGui에 삽입되는 뷰포트)
    ID3D11Texture2D* ViewerTexture = nullptr;
    ID3D11ShaderResourceView* ViewerSRV = nullptr;
    uint32 ViewerTextureWidth = 0;
    uint32 ViewerTextureHeight = 0;
    bool bViewportTextureValid = false;

    // Anim notify selection
    int32 SelectedNotifyIndex = -1;
    UAnimNotify* SelectedNotifyObject = nullptr;
};
