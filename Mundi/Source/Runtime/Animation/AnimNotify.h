#include "Object.h"
#include "SkeletalMeshComponent.h"
#include "AnimSequenceBase.h"

class UAnimNotify : public UObject
{
public:
	DECLARE_CLASS(UAnimNotify, UObject)

	UAnimNotify() = default;
	virtual ~UAnimNotify() override = default;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) {};
};
