#include "pch.h"
#include "AnimSequenceBase.h"

#include "FBXLoader.h"

IMPLEMENT_CLASS(UAnimSequenceBase)

void UAnimSequenceBase::Load(const FString& InFilePath, class ID3D11Device* InDevice)
{
	DataModel = UFbxLoader::GetInstance().LoadAnimationFromFbx(InFilePath, 0);
	if(!DataModel)
	{
		UE_LOG("UAnimSequenceBase::Load: Failed to load animation data model from FBX: %s", InFilePath.c_str());
	}
}
