#pragma once

class UWorld; class FViewport; class FViewportClient; class ASkeletalMeshActor; class USkeletalMesh;

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
    std::set<int32> ExpandedBoneIndices;

    // 본 트랜스폼 편집 관련
    FVector EditBoneLocation;
    FVector EditBoneRotation;  // Euler angles in degrees
    FVector EditBoneScale;
    
    bool bBoneTransformChanged = false;
    bool bBoneRotationEditing = false;

	// 애니메이션 관련
	bool bAnimationMode = false; // false면 본 편집 모드(초기 상태 or Stop버튼 누른 상태), true면 애니메이션 재생 모드
	bool bOnStopPressed = false; // Stop 버튼이 눌렸는지 여부 (true에 대한 처리 완료 시, 바로 false로 변경)
};
