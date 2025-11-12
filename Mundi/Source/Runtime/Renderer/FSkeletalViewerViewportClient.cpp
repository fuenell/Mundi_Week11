#include "pch.h"
#include "FSkeletalViewerViewportClient.h"
#include"CameraActor.h"
#include "World.h"

FSkeletalViewerViewportClient::FSkeletalViewerViewportClient()
{
    // Default to perspective camera and a sane default view mode.
    // SkeletalViewerBootstrap에서 바뀌고 있음
    ViewportType = EViewportType::Perspective;
    ViewMode = EViewMode::VMI_Lit_Phong;

    // Set initial camera position to front view for skeletal mesh viewer
    // Looking at origin (0,0,0) from front, slightly elevated

	Camera->SetActorLocation(FVector(200.0f, 0.0f, 75.0f)); // Front view, slightly elevated
    Camera->SetActorRotation(FVector(0.0f, 0.0f, 180.0f)); // Front view, slightly elevated

    // Initialize camera rotation state to match the initial rotation
    // This prevents the camera from snapping when first clicked
    Camera->SetCameraPitch(0.0f);   // Pitch: 0 degrees (level)
    Camera->SetCameraYaw(180.0f);   // Yaw: 180 degrees (facing +X direction from -X position)
}

FSkeletalViewerViewportClient::~FSkeletalViewerViewportClient()
{
}

void FSkeletalViewerViewportClient::Draw(FViewport* Viewport)
{
    // Use the base implementation which:
    // - Configures camera based on ViewportType
    // - Builds FSceneView from camera + Viewport
    // - Applies World->GetRenderSettings().SetViewMode(ViewMode)
    // - Calls Renderer->RenderSceneForView(World, &View, Viewport)
    FViewportClient::Draw(Viewport);
}

void FSkeletalViewerViewportClient::Tick(float DeltaTime)
{
    // Reuse base behavior (camera input, wheel, clipboard shortcuts)
    FViewportClient::Tick(DeltaTime);
}

void FSkeletalViewerViewportClient::MouseMove(FViewport* Viewport, int32 X, int32 Y)
{
    FViewportClient::MouseMove(Viewport, X, Y);
}

void FSkeletalViewerViewportClient::MouseButtonDown(FViewport* Viewport, int32 X, int32 Y, int32 Button)
{
    FViewportClient::MouseButtonDown(Viewport, X, Y, Button);
}

void FSkeletalViewerViewportClient::MouseButtonUp(FViewport* Viewport, int32 X, int32 Y, int32 Button)
{
    FViewportClient::MouseButtonUp(Viewport, X, Y, Button);
}

void FSkeletalViewerViewportClient::MouseWheel(float DeltaSeconds)
{
    FViewportClient::MouseWheel(DeltaSeconds);
}

