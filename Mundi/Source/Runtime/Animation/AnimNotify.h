#pragma once
#include "Object.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;

class UAnimNotify : public UObject
{
public:
	DECLARE_CLASS(UAnimNotify, UObject)

	UAnimNotify() = default;
	virtual ~UAnimNotify() override = default;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
};
