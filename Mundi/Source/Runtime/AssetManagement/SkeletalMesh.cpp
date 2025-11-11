#include "pch.h"
#include "SkeletalMesh.h"


#include "FbxLoader.h"
#include "WindowsBinReader.h"
#include "WindowsBinWriter.h"
#include "PathUtils.h"
#include <filesystem>

IMPLEMENT_CLASS(USkeletalMesh)

USkeletalMesh::USkeletalMesh()
{
}

USkeletalMesh::~USkeletalMesh()
{
    ReleaseResources();
}

void USkeletalMesh::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    if (Data)
    {
        ReleaseResources();
    }
    FString NormalizedPath = NormalizePath(InFilePath);

#ifdef USE_OBJ_CACHE
    // 1. 캐시 파일 경로 설정
    FString CachePathStr = ConvertDataPathToCachePath(NormalizedPath);
    const FString BinPathFileName = CachePathStr + ".bin";

    // 캐시를 저장할 디렉토리가 없으면 생성
    std::filesystem::path CacheFileDirPath(BinPathFileName);
    if (CacheFileDirPath.has_parent_path())
    {
        std::filesystem::create_directories(CacheFileDirPath.parent_path());
    }

    Data = new FSkeletalMeshData();
    bool bLoadedFromCache = false;

    // 2. 캐시 유효성 검사
    bool bShouldRegenerate = true;
    if (std::filesystem::exists(BinPathFileName))
    {
        try
        {
            auto binTime = std::filesystem::last_write_time(BinPathFileName);
            auto fbxTime = std::filesystem::last_write_time(NormalizedPath);

            // FBX 파일이 캐시보다 오래되었으면 캐시 사용
            if (fbxTime <= binTime)
            {
                bShouldRegenerate = false;
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            UE_LOG("Filesystem error during cache validation: %s. Forcing regeneration.", e.what());
            bShouldRegenerate = true;
        }
    }

    // 3. 캐시에서 로드 시도
    if (!bShouldRegenerate)
    {
        UE_LOG("Attempting to load SkeletalMesh '%s' from cache.", NormalizedPath.c_str());
        try
        {
            FWindowsBinReader Reader(BinPathFileName);
            if (!Reader.IsOpen())
            {
                throw std::runtime_error("Failed to open bin file for reading.");
            }
            Reader << *Data;
            Reader.Close();

            Data->CacheFilePath = BinPathFileName;
            bLoadedFromCache = true;

            UE_LOG("Successfully loaded SkeletalMesh '%s' from cache.", NormalizedPath.c_str());
        }
        catch (const std::exception& e)
        {
            UE_LOG("Error loading SkeletalMesh from cache: %s. Cache might be corrupt or incompatible.", e.what());
            UE_LOG("Deleting corrupt cache and forcing regeneration for '%s'.", NormalizedPath.c_str());

            std::filesystem::remove(BinPathFileName);
            delete Data;
            bLoadedFromCache = false;
        }
    }

    // 4. 캐시 로드 실패 시 FBX 파싱
    if (!bLoadedFromCache)
    {
        UE_LOG("Regenerating cache for SkeletalMesh '%s'...", NormalizedPath.c_str());

        Data = UFbxLoader::GetInstance().LoadFbxMeshAsset(NormalizedPath);
        if (Data->Vertices.empty() || Data->Indices.empty())
        {
            UE_LOG("ERROR: Failed to load FBX mesh from '%s'", NormalizedPath.c_str());
            return;
        }

        // 캐시 저장
        try
        {
            FWindowsBinWriter Writer(BinPathFileName);
            Writer << *Data;
            Writer.Close();

            Data->CacheFilePath = BinPathFileName;

            UE_LOG("Cache regeneration complete for SkeletalMesh '%s'.", NormalizedPath.c_str());
        }
        catch (const std::exception& e)
        {
            UE_LOG("Failed to save SkeletalMesh cache: %s", e.what());
        }
    }
#else
    // 캐싱 비활성화 시 항상 FBX 파싱
    Data = UFbxLoader::GetInstance().LoadFbxMeshAsset(NormalizedPath);
    if (Data.Vertices.empty() || Data.Indices.empty())
    {
        UE_LOG("ERROR: Failed to load FBX mesh from '%s'", NormalizedPath.c_str());
        return;
    }
#endif // USE_OBJ_CACHE

    // 5. GPU 버퍼 생성
    CreateVertexBuffer(Data, InDevice);
    CreateIndexBuffer(Data, InDevice);
    VertexCount = static_cast<uint32>(Data->Vertices.size());
    IndexCount = static_cast<uint32>(Data->Indices.size());
    VertexStride = sizeof(FVertexDynamic);
}

void USkeletalMesh::ReleaseResources()
{
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }

    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }

    if (Data)
    {
        delete Data;
        Data = nullptr;
    }
}

void USkeletalMesh::SetSkeletalMeshAsset(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice)
{
    if (Data)
    {
        ReleaseResources();
    }
    Data = InSkeletalMesh;

    CreateVertexBuffer(Data, InDevice);
    CreateIndexBuffer(Data, InDevice);
    VertexCount = static_cast<uint32>(Data->Vertices.size());
    IndexCount = static_cast<uint32>(Data->Indices.size());
}

void USkeletalMesh::UpdateVertexBuffer(const TArray<FNormalVertex>& SkinnedVertices)
{
    if (!VertexBuffer) { return; }

    GEngine.GetRHIDevice()->VertexBufferUpdate(VertexBuffer, SkinnedVertices);
}

void USkeletalMesh::CreateVertexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice)
{
    HRESULT hr = D3D11RHI::CreateVertexBuffer<FVertexDynamic>(InDevice, InSkeletalMesh->Vertices, &VertexBuffer);
    assert(SUCCEEDED(hr));
}

void USkeletalMesh::CreateIndexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice)
{
    HRESULT hr = D3D11RHI::CreateIndexBuffer(InDevice, InSkeletalMesh, &IndexBuffer);
    assert(SUCCEEDED(hr));
}
