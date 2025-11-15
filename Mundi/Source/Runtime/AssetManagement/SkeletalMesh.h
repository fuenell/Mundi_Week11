#pragma once
#include "ResourceBase.h"

class USkeletalMesh : public UResourceBase
{
public:
    DECLARE_CLASS(USkeletalMesh, UResourceBase)

    USkeletalMesh();
    virtual ~USkeletalMesh() override;
    
    void Load(const FString& InFilePath, ID3D11Device* InDevice);
    
    const FSkeletalMeshData* GetSkeletalMeshData() const { return Data; }
    const FString& GetPathFileName() const { static const FString EmptyString; if (Data) return Data->PathFileName; return EmptyString; }
    const FSkeleton* GetSkeleton() const { return Data ? &Data->Skeleton : nullptr; }
    uint32 GetBoneCount() const { return Data ? Data->Skeleton.Bones.Num() : 0; }
    
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }

    uint32 GetVertexCount() const { return VertexCount; }
    uint32 GetIndexCount() const { return IndexCount; }

    uint32 GetVertexStride() const { return VertexStride; }
    
    const TArray<FGroupInfo>& GetMeshGroupInfo() const { static TArray<FGroupInfo> EmptyGroup; return Data ? Data->GroupInfos : EmptyGroup; }
    bool HasMaterial() const { return Data ? Data->bHasMaterial : false; }

    uint64 GetMeshGroupCount() const { return Data ? Data->GroupInfos.size() : 0; }

    void CreateVertexBuffer(ID3D11Buffer** InVertexBuffer, ESkinningMode Skinning);
    void UpdateVertexBuffer(const TArray<FNormalVertex>& SkinnedVertices, ID3D11Buffer* InVertexBuffer);
    
private:
    void CreateIndexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice);
    void ReleaseResources();
    
private:
	ID3D11Buffer* VertexBuffer = nullptr; // GPU 스키닝용 버텍스 버퍼
    ID3D11Buffer* IndexBuffer = nullptr;  // CPU, GPU 스키닝 공용 인덱스 버퍼
    uint32 VertexCount = 0;               // 정점 개수
    uint32 IndexCount = 0;                // 버텍스 점의 개수 
    uint32 VertexStride = 0;
    
    // CPU 리소스
    FSkeletalMeshData* Data = nullptr;
};
