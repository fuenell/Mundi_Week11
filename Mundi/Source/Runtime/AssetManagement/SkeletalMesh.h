#pragma once
#include "ResourceBase.h"

class USkeletalMesh : public UResourceBase
{
public:
    DECLARE_CLASS(USkeletalMesh, UResourceBase)

    USkeletalMesh();
    virtual ~USkeletalMesh() override;
    
    void Load(const FString& InFilePath, ID3D11Device* InDevice);
    
    const FSkeletalMeshData* GetSkeletalMeshData() const { return &Data; }
    const FSkeleton* GetSkeleton() const { return &Data.Skeleton; }
    const TArray<FGroupInfo>& GetGroupInfos() const { return Data.GroupInfos; }
    uint32 GetBoneCount() const { return Data.Skeleton.Bones.Num(); }
    
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }

    uint32 GetVertexCount() const { return VertexCount; }
    uint32 GetIndexCount() const { return IndexCount; }

    uint32 GetVertexStride() const { return VertexStride; }

    void SetSkeletalMeshAsset(FSkeletalMeshData* InStaticMesh) { Data = *InStaticMesh; }
    const FSkeletalMeshData* GetSkeletalMeshAsset() const { return &Data; }

    const TArray<FGroupInfo>& GetMeshGroupInfo() const { return Data.GroupInfos; }
    bool HasMaterial() const { return Data.bHasMaterial; }

    uint64 GetMeshGroupCount() const { return Data.GroupInfos.size(); }

    void UpdateVertexBuffer(const TArray<FNormalVertex>& SkinnedVertices);
    
private:
    void CreateVertexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice);
    void CreateIndexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice);
    void ReleaseResources();
    
private:
    // GPU 리소스
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32 VertexCount = 0;     // 정점 개수
    uint32 IndexCount = 0;     // 버텍스 점의 개수 
    uint32 VertexStride = 0;
    
    // CPU 리소스
    FSkeletalMeshData Data;
};