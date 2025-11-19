#include "pch.h"
#include "AnimNotify.h"
#include "SkeletalMeshComponent.h"
#include "AnimSequenceBase.h"

IMPLEMENT_CLASS(UAnimNotify)

void UAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// 기본 구현은 아무 작업도 수행하지 않음
}
