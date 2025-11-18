#include "pch.h"
#include "AnimSequenceBase.h"

#include "FBXLoader.h"

IMPLEMENT_CLASS(UAnimSequenceBase)

void UAnimSequenceBase::Load(const FString& InFilePath, class ID3D11Device* InDevice)
{
	//DataModel = UFbxLoader::GetInstance().LoadAnimationFromFbx(InFilePath, 0);
	DataModel = UFbxLoader::GetInstance().LoadAnimationMixamo(InFilePath);
	if(!DataModel)
	{
		UE_LOG("UAnimSequenceBase::Load: Failed to load animation data model from FBX OR No Animation: %s", InFilePath.c_str());
	}

	SetFilePath(InFilePath);
}
