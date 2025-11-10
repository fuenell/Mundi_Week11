#pragma once
#include <string>

class UWorld; class FViewport; class FViewportClient; class ASkinnedMeshActor; class USkeletalMesh;

class ViewerState
{
public:
    std::string Name;
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;
    
    // Have a pointer to the currently selected mesh to render in the viewer
    ASkinnedMeshActor* PreviewActor = nullptr;
    int32 SelectedBoneIndex = -1;
    bool bShowMesh = true;
    bool bShowBones = true;
};
