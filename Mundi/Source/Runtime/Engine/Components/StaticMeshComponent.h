#pragma once

#include "MeshComponent.h"
#include "AABB.h"
#include "UStaticMeshComponent.generated.h"

class UStaticMesh;
class UShader;
class UTexture;
class UMaterialInterface;
class UMaterialInstanceDynamic;
struct FSceneCompData;

UCLASS(DisplayName="스태틱 메시 컴포넌트", Description="정적 메시를 렌더링하는 컴포넌트입니다")
class UStaticMeshComponent : public UMeshComponent
{
public:

	GENERATED_REFLECTION_BODY()
	REGISTER_PRIMITIVE_COMPONENT(UStaticMeshComponent)

	UStaticMeshComponent();

protected:
	~UStaticMeshComponent() override;

public:
	void OnStaticMeshReleased(UStaticMesh* ReleasedMesh);

	void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetStaticMesh(const FString& PathFileName);

	UStaticMesh* GetStaticMesh() const { return StaticMesh; }
	
	FAABB GetWorldAABB() const override;

	void DuplicateSubObjects() override;

protected:
	void OnTransformUpdated() override;

protected:
	UPROPERTY(EditAnywhere, Category="Static Mesh", Tooltip="Static mesh asset to render")
	UStaticMesh* StaticMesh = nullptr;
};
