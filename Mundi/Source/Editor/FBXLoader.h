#pragma once
#include "Object.h"
#include "fbxsdk.h"

class UAnimDataModel; // 전방 선언
class USkeletalMesh;
struct FSkeletalMeshData;
struct FMaterialInfo;
struct FBoneAnimationTrack;
struct FVectorCurve;
struct FFloatCurve;

class UFbxLoader : public UObject
{
public:
	DECLARE_CLASS(UFbxLoader, UObject)
	
	static UFbxLoader& GetInstance();
	UFbxLoader();

	static void PreLoad();

	USkeletalMesh* LoadFbxMesh(const FString& FilePath);
	FSkeletalMeshData* LoadFbxMeshAsset(const FString& FilePath);

	// 애니메이션 추출 함수
	UAnimDataModel* LoadAnimationFromFbx(const FString& FilePath, int32 AnimStackIndex = 0);

protected:
	~UFbxLoader() override;

private:
	UFbxLoader(const UFbxLoader&) = delete;
	UFbxLoader& operator=(const UFbxLoader&) = delete;

	// 메시 로딩 관련 함수들
	void LoadMeshFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TMap<FbxSurfaceMaterial*, int32>& MaterialToIndex);
	void LoadSkeletonFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, int32 ParentNodeIndex, TMap<FbxNode*, int32>& BoneToIndex);
	void LoadMeshFromAttribute(FbxNodeAttribute* InAttribute, FSkeletalMeshData& MeshData);
	void LoadMesh(FbxMesh* InMesh, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TArray<int32> MaterialSlotToIndex, int32 DefaultMaterialIndex = 0);
	void ParseMaterial(FbxSurfaceMaterial* Material, FMaterialInfo& MaterialInfo);
	FString ParseTexturePath(FbxProperty& Property);
	FbxString GetAttributeTypeName(FbxNodeAttribute* InAttribute);
	void EnsureSingleRootBone(FSkeletalMeshData& MeshData);

	// 애니메이션 추출 관련 함수들
	void ExtractBoneAnimation(FbxNode* InNode, FbxAnimLayer* InAnimLayer, const TMap<FbxNode*, int32>& BoneToIndex, 
	                          FbxTime StartTime, FbxTime EndTime, FbxTime FrameTime, 
	                          UAnimDataModel* OutAnimData);

	void ExtractBoneAnimationAdditiveLayer(FbxNode* InNode, FbxAnimLayer* InAnimLayer, 
	                                       const TMap<FbxNode*, int32>& BoneToIndex,
	                                       FbxTime StartTime, FbxTime EndTime, FbxTime FrameTime,
	                                       UAnimDataModel* OutAnimData, int32 LayerIndex);

	void BlendAnimationTracks(FBoneAnimationTrack& BaseTrack, const FBoneAnimationTrack& AdditiveTrack);

	void ExtractAnimationCurves(FbxNode* InNode, FbxAnimLayer* InAnimLayer, UAnimDataModel* OutAnimData, int32 LayerIndex = 0);
	
	void ExtractVectorCurve(FbxAnimCurve* CurveX, FbxAnimCurve* CurveY, FbxAnimCurve* CurveZ,
	                        FVectorCurve& OutVectorCurve, const char* NodeName, const char* CurveType);
	
	void ExtractFloatCurveFromFbx(FbxAnimCurve* FbxCurve, FFloatCurve& OutFloatCurve);
	
	void ExtractFloatCurves(FbxScene* Scene, FbxAnimLayer* InAnimLayer, UAnimDataModel* OutAnimData, int32 LayerIndex = 0);
	
	void ExtractFloatCurvesRecursive(FbxNode* InNode, FbxAnimLayer* InAnimLayer, UAnimDataModel* OutAnimData, int32 LayerIndex = 0);
	
	void ExtractMorphTargetCurves(FbxMesh* Mesh, FbxNode* InNode, FbxAnimLayer* InAnimLayer, UAnimDataModel* OutAnimData, int32 LayerIndex = 0);

	// 멤버 변수
	TArray<FMaterialInfo> MaterialInfos;
	FbxManager* SdkManager = nullptr;
	FString CurrentFbxBaseDir;
};
