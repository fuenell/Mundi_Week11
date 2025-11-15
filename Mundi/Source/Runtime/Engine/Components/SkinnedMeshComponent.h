#pragma once
#include "MeshComponent.h"
#include "SkeletalMesh.h"
#include "USkinnedMeshComponent.generated.h"

UCLASS(DisplayName="스킨드 메시 컴포넌트", Description="스켈레탈 메시를 렌더링하는 컴포넌트입니다")
class USkinnedMeshComponent : public UMeshComponent
{
public:
    GENERATED_REFLECTION_BODY()

    USkinnedMeshComponent();
    ~USkinnedMeshComponent() override;

    void BeginPlay() override;
    void TickComponent(float DeltaTime) override;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    void DuplicateSubObjects() override;
    
// Mesh Component Section
public:
    void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;
    
    FAABB GetWorldAABB() const override;
    void OnTransformUpdated() override;

// Skeletal Section
public:
    /**
     * @brief 렌더링할 스켈레탈 메시 에셋 설정 (UStaticMeshComponent::SetStaticMesh와 동일한 역할)
     * @param PathFileName 새 스켈레탈 메시 에셋 경로
     */
    virtual void SetSkeletalMesh(const FString& PathFileName);
    /**
     * @brief 이 컴포넌트의 USkeletalMesh 에셋을 반환
     */
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

protected:
    void PerformCpuSkinning();
    /**
     * @brief 자식에게서 원본 메시를 받아 CPU 스키닝을 수행
     * @param InSkinningMatrices 스키닝 매트릭스
     */
    void UpdateSkinningMatrices(const TArray<FMatrix>& InSkinningMatrices, const TArray<FMatrix>& InSkinningNormalMatrices);
    
    UPROPERTY(EditAnywhere, Category = "Skeletal Mesh", Tooltip = "Skeletal mesh asset to render")
    USkeletalMesh* SkeletalMesh;

    /**
     * @brief CPU 스키닝 최종 결과물. 렌더러가 이 데이터를 사용합니다.
     */
    TArray<FNormalVertex> SkinnedVertices;

private:
	// CPU Skinning Methods
    FVector SkinVertexPosition(const FSkinnedVertex& InVertex) const;
    FVector SkinVertexNormal(const FSkinnedVertex& InVertex) const;
    FVector4 SkinVertexTangent(const FSkinnedVertex& InVertex) const;

	// GPU Skinning Resources
    void CreateSkinningMatrixResources(uint32 InBoneCount);
    void ReleaseSkinningMatrixResources();

    /**
     * @brief 자식이 계산해 준, 현재 프레임의 최종 스키닝 행렬
    */
    TArray<FMatrix> FinalSkinningMatrices;
    TArray<FMatrix> FinalSkinningNormalMatrices;
    bool bSkinningMatricesDirty = true;
    
    /**
     * @brief CPU 스키닝용 Component별 VertexBuffer
    */
    ID3D11Buffer* VertexBuffer = nullptr;

	/**
	 * @brief GPU 스키닝용 StructuredBuffer 및 SRV
	*/
    ID3D11Buffer* SkinningMatrixBuffer = nullptr;
    ID3D11ShaderResourceView* SkinningMatrixSRV = nullptr;
    uint32 SkinningMatrixCount = 0;  // 현재 컴포넌트의 본 수
    uint32 SkinningMatrixOffset = 0; // 행렬 풀을 공유하는 구조로 확장할 경우 오프셋으로 활용
};
