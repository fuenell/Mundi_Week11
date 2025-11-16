#include "pch.h"
#include "SkinnedMeshComponent.h"
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "D3D11RHI.h"
#include "VertexData.h"
#include "World.h"

USkinnedMeshComponent::USkinnedMeshComponent() : SkeletalMesh(nullptr)
{
	bCanEverTick = true;
}

USkinnedMeshComponent::~USkinnedMeshComponent()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}
	ReleaseSkinningMatrixResources();
}

void USkinnedMeshComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USkinnedMeshComponent::TickComponent(float DeltaTime)
{
	UMeshComponent::TickComponent(DeltaTime);
}

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		SetSkeletalMesh(SkeletalMesh->GetPathFileName());
	}
	// @TODO - UStaticMeshComponent처럼 프로퍼티 기반 직렬화 로직 추가
}

void USkinnedMeshComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	if (!SkeletalMesh)
	{
		return;
	}

	SkeletalMesh->CreateVertexBuffer(&VertexBuffer, ESkinningMode::CPU);

	// 얕은 복사된 GPU 리소스 포인터는 원본 컴포넌트를 가리키고 있으므로,
	// 새 버퍼를 만들기 전에 참조를 끊어 원본이 해제되지 않도록 한다.
	SkinningMatrixBuffer = nullptr;
	SkinningMatrixSRV = nullptr;
	SkinningNormalMatrixBuffer = nullptr;
	SkinningNormalMatrixSRV = nullptr;
	SkinningMatrixCount = 0;
	SkinningMatrixOffset = 0;

	const uint32 BoneCount = SkeletalMesh->GetBoneCount();
	if (BoneCount == 0)
	{
		return;
	}

	CreateSkinningMatrixResources(BoneCount);

	// 원본으로부터 얕게 복사된 스키닝 행렬 데이터를 새 버퍼에 업로드하여
	// PIE 복사본이 독립적인 GPU 리소스를 갖도록 만든다.
	if (!FinalSkinningMatrices.IsEmpty() &&
		!FinalSkinningNormalMatrices.IsEmpty() &&
		SkinningMatrixBuffer && SkinningNormalMatrixBuffer)
	{
		const uint32 MatrixCount = FMath::Min<uint32>(BoneCount, FinalSkinningMatrices.Num());
		const uint32 NormalMatrixCount = FMath::Min<uint32>(BoneCount, FinalSkinningNormalMatrices.Num());

		if (D3D11RHI* RHIDevice = GEngine.GetRHIDevice())
		{
			if (MatrixCount > 0)
			{
				RHIDevice->UpdateStructuredBuffer(
					SkinningMatrixBuffer,
					FinalSkinningMatrices.data(),
					sizeof(FMatrix) * MatrixCount);
			}

			if (NormalMatrixCount > 0)
			{
				RHIDevice->UpdateStructuredBuffer(
					SkinningNormalMatrixBuffer,
					FinalSkinningNormalMatrices.data(),
					sizeof(FMatrix) * NormalMatrixCount);
			}
		}
	}
}

void USkinnedMeshComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData()) { return; }

	const ESkinningMode SkinningModeToUse =
		(View && View->RenderSettings) ? View->RenderSettings->GetSkinningMode() : ESkinningMode::CPU;

	if (SkinningModeToUse == ESkinningMode::CPU && bSkinningMatricesDirty)
	{
		bSkinningMatricesDirty = false;
		SkeletalMesh->UpdateVertexBuffer(SkinnedVertices, VertexBuffer);
	}

	const TArray<FGroupInfo>& MeshGroupInfos = SkeletalMesh->GetMeshGroupInfo();
	auto DetermineMaterialAndShader = [&](uint32 SectionIndex) -> TPair<UMaterialInterface*, UShader*>
	{
		UMaterialInterface* Material = GetMaterial(SectionIndex);
		UShader* Shader = nullptr;

		if (Material && Material->GetShader())
		{
			Shader = Material->GetShader();
		}
		else
		{
			// UE_LOG("USkinnedMeshComponent: 머티리얼이 없거나 셰이더가 없어서 기본 머티리얼 사용 section %u.", SectionIndex);
			Material = UResourceManager::GetInstance().GetDefaultMaterial();
			if (Material)
			{
				Shader = Material->GetShader();
			}
			if (!Material || !Shader)
			{
				UE_LOG("USkinnedMeshComponent: 기본 머티리얼이 없습니다.");
				return { nullptr, nullptr };
			}
		}
		return { Material, Shader };
	};

	const bool bHasSections = !MeshGroupInfos.IsEmpty();
	const uint32 NumSectionsToProcess = bHasSections ? static_cast<uint32>(MeshGroupInfos.size()) : 1;

	for (uint32 SectionIndex = 0; SectionIndex < NumSectionsToProcess; ++SectionIndex)
	{
		uint32 IndexCount = 0;
		uint32 StartIndex = 0;

		if (bHasSections)
		{
			const FGroupInfo& Group = MeshGroupInfos[SectionIndex];
			IndexCount = Group.IndexCount;
			StartIndex = Group.StartIndex;
		}
		else
		{
			IndexCount = SkeletalMesh->GetIndexCount();
			StartIndex = 0;
		}

		if (IndexCount == 0)
		{
			continue;
		}

		auto [MaterialToUse, ShaderToUse] = DetermineMaterialAndShader(SectionIndex);
		if (!MaterialToUse || !ShaderToUse)
		{
			continue;
		}

		FMeshBatchElement BatchElement;
		TArray<FShaderMacro> ShaderMacros = View->ViewShaderMacros;
		if (0 < MaterialToUse->GetShaderMacros().Num())
		{
			ShaderMacros.Append(MaterialToUse->GetShaderMacros());
		}
		if (SkinningModeToUse == ESkinningMode::GPU)
		{
			ShaderMacros.Add(FShaderMacro("USE_GPU_SKINNING", "1"));
		}
		FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ShaderMacros);

		if (ShaderVariant)
		{
			BatchElement.VertexShader = ShaderVariant->VertexShader;
			BatchElement.PixelShader = ShaderVariant->PixelShader;
			BatchElement.InputLayout = ShaderVariant->InputLayout;
		}
		
		BatchElement.Material = MaterialToUse;
		
		BatchElement.SkinningMode = SkinningModeToUse;
		if (SkinningModeToUse == ESkinningMode::GPU)
		{
			BatchElement.VertexBuffer = SkeletalMesh->GetVertexBuffer();
			BatchElement.VertexStride = sizeof(FSkinnedVertex);
			BatchElement.SkinningMatrixSRV = SkinningMatrixSRV;
			BatchElement.SkinningNormalMatrixSRV = SkinningNormalMatrixSRV;
			BatchElement.SkinningMatrixOffset = SkinningMatrixOffset;
			BatchElement.SkinningMatrixCount = SkinningMatrixCount;
		}
		else
		{
			BatchElement.VertexBuffer = VertexBuffer;
			BatchElement.VertexStride = SkeletalMesh->GetVertexStride();
			BatchElement.SkinningMatrixSRV = nullptr;
			BatchElement.SkinningNormalMatrixSRV = nullptr;
			BatchElement.SkinningMatrixOffset = 0;
			BatchElement.SkinningMatrixCount = 0;
		}
		BatchElement.IndexBuffer = SkeletalMesh->GetIndexBuffer();
		
		BatchElement.IndexCount = IndexCount;
		BatchElement.StartIndex = StartIndex;
		BatchElement.BaseVertexIndex = 0;
		BatchElement.WorldMatrix = GetWorldMatrix();
		BatchElement.ObjectID = InternalIndex;
		BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		OutMeshBatchElements.Add(BatchElement);
	}
}

FAABB USkinnedMeshComponent::GetWorldAABB() const
{
	return {};
	// const FTransform WorldTransform = GetWorldTransform();
	// const FMatrix WorldMatrix = GetWorldMatrix();
	//
	// if (!SkeletalMesh)
	// {
	//    const FVector Origin = WorldTransform.TransformPosition(FVector());
	//    return FAABB(Origin, Origin);
	// }
	//
	// const FAABB LocalBound = SkeletalMesh->GetLocalBound(); // <-- 이 함수 구현 필요
	// const FVector LocalMin = LocalBound.Min;
	// const FVector LocalMax = LocalBound.Max;
	//
	// // ... (이하 AABB 계산 로직은 UStaticMeshComponent와 동일) ...
	// const FVector LocalCorners[8] = {
	//    FVector(LocalMin.X, LocalMin.Y, LocalMin.Z),
	//    FVector(LocalMax.X, LocalMin.Y, LocalMin.Z),
	//    // ... (나머지 6개 코너) ...
	//    FVector(LocalMax.X, LocalMax.Y, LocalMax.Z)
	// };
	//
	// FVector4 WorldMin4 = FVector4(LocalCorners[0].X, LocalCorners[0].Y, LocalCorners[0].Z, 1.0f) * WorldMatrix;
	// FVector4 WorldMax4 = WorldMin4;
	//
	// for (int32 CornerIndex = 1; CornerIndex < 8; ++CornerIndex)
	// {
	//    const FVector4 WorldPos = FVector4(LocalCorners[CornerIndex].X
	//       , LocalCorners[CornerIndex].Y
	//       , LocalCorners[CornerIndex].Z
	//       , 1.0f)
	//       * WorldMatrix;
	//    WorldMin4 = WorldMin4.ComponentMin(WorldPos);
	//    WorldMax4 = WorldMax4.ComponentMax(WorldPos);
	// }
	//
	// FVector WorldMin = FVector(WorldMin4.X, WorldMin4.Y, WorldMin4.Z);
	// FVector WorldMax = FVector(WorldMax4.X, WorldMax4.Y, WorldMax4.Z);
	// return FAABB(WorldMin, WorldMax);
}

void USkinnedMeshComponent::OnTransformUpdated()
{
	Super::OnTransformUpdated();
	MarkWorldPartitionDirty();
}

void USkinnedMeshComponent::SetSkeletalMesh(const FString& PathFileName)
{
	ClearDynamicMaterials();

	SkeletalMesh = UResourceManager::GetInstance().Load<USkeletalMesh>(PathFileName);

	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}
	ReleaseSkinningMatrixResources();
	
	if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
	{
		SkeletalMesh->CreateVertexBuffer(&VertexBuffer, ESkinningMode::CPU);

		const uint32 BoneCount = SkeletalMesh->GetBoneCount();
		const TArray<FMatrix> IdentityMatrices(BoneCount, FMatrix::Identity());
		UpdateSkinningMatrices(IdentityMatrices, IdentityMatrices);
		PerformCpuSkinning();
		CreateSkinningMatrixResources(BoneCount);
		
		const TArray<FGroupInfo>& GroupInfos = SkeletalMesh->GetMeshGroupInfo();
		MaterialSlots.resize(GroupInfos.size());
		for (int i = 0; i < GroupInfos.size(); ++i)
		{
			// FGroupInfo에 InitialMaterialName이 있다고 가정
			SetMaterialByName(i, GroupInfos[i].InitialMaterialName);
		}
		MarkWorldPartitionDirty();
	}
	else
	{
		SkeletalMesh = nullptr;
		UpdateSkinningMatrices(TArray<FMatrix>(), TArray<FMatrix>());
		PerformCpuSkinning();
		ReleaseSkinningMatrixResources();
	}
}

void USkinnedMeshComponent::PerformCpuSkinning()
{
	if (!SkeletalMesh || FinalSkinningMatrices.IsEmpty()) { return; }
	const UWorld* World = GetWorld();
	const ESkinningMode CurrentSkinningMode =
		(World ? World->GetRenderSettings().GetSkinningMode() : ESkinningMode::CPU);
	if (CurrentSkinningMode != ESkinningMode::CPU)
	{
		return;
	}
	if (!bSkinningMatricesDirty) { return; }
	
	const TArray<FSkinnedVertex>& SrcVertices = SkeletalMesh->GetSkeletalMeshData()->Vertices;
	const int32 NumVertices = SrcVertices.Num();
	SkinnedVertices.SetNum(NumVertices);

	for (int32 Idx = 0; Idx < NumVertices; ++Idx)
	{
		const FSkinnedVertex& SrcVert = SrcVertices[Idx];
		FNormalVertex& DstVert = SkinnedVertices[Idx];

		DstVert.pos = SkinVertexPosition(SrcVert); 
		DstVert.normal = SkinVertexNormal(SrcVert);
		DstVert.Tangent = SkinVertexTangent(SrcVert);
		DstVert.tex = SrcVert.UV;
	}
}

void USkinnedMeshComponent::UpdateSkinningMatrices(const TArray<FMatrix>& InSkinningMatrices, const TArray<FMatrix>& InSkinningNormalMatrices)
{
	FinalSkinningMatrices = InSkinningMatrices;
	FinalSkinningNormalMatrices = InSkinningNormalMatrices;
	bSkinningMatricesDirty = true;

	const UWorld* World = GetWorld();
	if (!World || World->GetRenderSettings().GetSkinningMode() != ESkinningMode::GPU)
	{
		return;
	}

	if (!SkinningMatrixBuffer || !SkinningNormalMatrixBuffer || FinalSkinningMatrices.IsEmpty() || FinalSkinningNormalMatrices.IsEmpty())
	{
		return;
	}

	const uint32 BoneCount = static_cast<uint32>(FinalSkinningMatrices.Num());
	if (BoneCount > SkinningMatrixCount)
	{
		UE_LOG("USkinnedMeshComponent: Structured buffer 크기가 부족합니다. 스켈레톤 본 개수가 예상 외로 변경되었을 수 있습니다.");
		return;
	}

	if (D3D11RHI* RHIDevice = GEngine.GetRHIDevice())
	{
		RHIDevice->UpdateStructuredBuffer(
			SkinningMatrixBuffer,
			FinalSkinningMatrices.data(),
			sizeof(FMatrix) * BoneCount);
		RHIDevice->UpdateStructuredBuffer(
			SkinningNormalMatrixBuffer,
			FinalSkinningNormalMatrices.data(),
			sizeof(FMatrix) * BoneCount);
	}
}

FVector USkinnedMeshComponent::SkinVertexPosition(const FSkinnedVertex& InVertex) const
{
	FVector BlendedPosition(0.f, 0.f, 0.f);

	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const uint32 BoneIndex = InVertex.BoneIndices[Idx];
		const float Weight = InVertex.BoneWeights[Idx];

		if (Weight > 0.f)
		{
			const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
			FVector TransformedPosition = SkinMatrix.TransformPosition(InVertex.Position);
			BlendedPosition += TransformedPosition * Weight;
		}
	}

	return BlendedPosition;
}

FVector USkinnedMeshComponent::SkinVertexNormal(const FSkinnedVertex& InVertex) const
{
	FVector BlendedNormal(0.f, 0.f, 0.f);

	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const uint32 BoneIndex = InVertex.BoneIndices[Idx];
		const float Weight = InVertex.BoneWeights[Idx];

		if (Weight > 0.f)
		{
			const FMatrix& SkinMatrix = FinalSkinningNormalMatrices[BoneIndex];
			FVector TransformedNormal = SkinMatrix.TransformVector(InVertex.Normal);
			BlendedNormal += TransformedNormal * Weight;
		}
	}

	return BlendedNormal.GetSafeNormal();
}

FVector4 USkinnedMeshComponent::SkinVertexTangent(const FSkinnedVertex& InVertex) const
{
	const FVector OriginalTangentDir(InVertex.Tangent.X, InVertex.Tangent.Y, InVertex.Tangent.Z);
	const float OriginalSignW = InVertex.Tangent.W;

	FVector BlendedTangentDir(0.f, 0.f, 0.f);

	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const uint32 BoneIndex = InVertex.BoneIndices[Idx];
		const float Weight = InVertex.BoneWeights[Idx];

		if (Weight > 0.f)
		{
			const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
			FVector TransformedTangentDir = SkinMatrix.TransformVector(OriginalTangentDir);
			BlendedTangentDir += TransformedTangentDir * Weight;
		}
	}

	const FVector FinalTangentDir = BlendedTangentDir.GetSafeNormal();
	return { FinalTangentDir.X, FinalTangentDir.Y, FinalTangentDir.Z, OriginalSignW };
}

void USkinnedMeshComponent::CreateSkinningMatrixResources(uint32 InBoneCount)
{
	ReleaseSkinningMatrixResources();

	if (InBoneCount == 0)
	{
		return;
	}

	D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
	if (!RHIDevice)
	{
		return;
	}

	HRESULT hr = RHIDevice->CreateStructuredBuffer(sizeof(FMatrix), InBoneCount, nullptr, &SkinningMatrixBuffer);
	assert(SUCCEEDED(hr));

	hr = RHIDevice->CreateStructuredBufferSRV(SkinningMatrixBuffer, &SkinningMatrixSRV);
	assert(SUCCEEDED(hr));

	hr = RHIDevice->CreateStructuredBuffer(sizeof(FMatrix), InBoneCount, nullptr, &SkinningNormalMatrixBuffer);
	assert(SUCCEEDED(hr));

	hr = RHIDevice->CreateStructuredBufferSRV(SkinningNormalMatrixBuffer, &SkinningNormalMatrixSRV);
	assert(SUCCEEDED(hr));

	SkinningMatrixCount = InBoneCount;
	SkinningMatrixOffset = 0;
}

void USkinnedMeshComponent::ReleaseSkinningMatrixResources()
{
	if (SkinningMatrixSRV)
	{
		SkinningMatrixSRV->Release();
		SkinningMatrixSRV = nullptr;
	}
	if (SkinningNormalMatrixSRV)
	{
		SkinningNormalMatrixSRV->Release();
		SkinningNormalMatrixSRV = nullptr;
	}

	if (SkinningMatrixBuffer)
	{
		SkinningMatrixBuffer->Release();
		SkinningMatrixBuffer = nullptr;
	}
	if (SkinningNormalMatrixBuffer)
	{
		SkinningNormalMatrixBuffer->Release();
		SkinningNormalMatrixBuffer = nullptr;
	}

	SkinningMatrixCount = 0;
	SkinningMatrixOffset = 0;
}
