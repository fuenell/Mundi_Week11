#include "pch.h"
#include "AnimSingleNodeInstance.h"
#include "AnimationAsset.h"
#include "AnimSequence.h"

IMPLEMENT_CLASS(UAnimSingleNodeInstance)

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping, float InPlayRate)
{
	if (NewAsset != CurrentAsset)
	{
		CurrentAsset = NewAsset;
		CurrentTime = 0.0f; // 새 애니메이션 시작 시 시간 초기화
	}
	bLooping = bIsLooping;
	PlayRate = InPlayRate;
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if(!bIsPlaying && !bDirtyTime)
	{
		// 재생도 멈추고 시간 변경도 없으면 아무 작업도 하지 않음
		return;
	}

	UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentAsset);
	if (CurrentAsset && AnimSequence)
	{
		// 2. 애니메이션 길이 가져오기
		UAnimDataModel* DataModel = AnimSequence->GetDataModel();
		if (!DataModel)
		{
			return;
		}

		if (bIsPlaying)
		{
			// 1. 현재 시간 업데이트
			CurrentTime += DeltaSeconds * PlayRate;

			float AnimLength = DataModel->GetPlayLength();

			// 3. 루핑 또는 클램핑 처리
			if (bLooping)
			{
				// 루핑: 애니메이션 길이를 넘으면 처음으로 되돌림
				if (AnimLength > 0.0f)
				{
					CurrentTime = fmodf(CurrentTime, AnimLength);
					if (CurrentTime < 0.0f)
					{
						CurrentTime += AnimLength;
					}
				}
			}
			else
			{
				// 논루핑: 애니메이션 길이를 넘으면 마지막 프레임에 고정
				if (CurrentTime >= AnimLength)
				{
					CurrentTime = AnimLength;
					bIsPlaying = false; // 애니메이션 종료
				}
			}
		}
		else if (bDirtyTime)
		{
			// 재생이 멈춘 상태에서 시간만 변경된 경우, 유효 범위로 클램핑
			float AnimLength = DataModel->GetPlayLength();
			if (CurrentTime < 0.0f)
			{
				CurrentTime = 0.0f;
			}
			else if (CurrentTime > AnimLength)
			{
				CurrentTime = AnimLength;
			}

			bDirtyTime = false;
		}


		// 4. 애니메이션 포즈 추출을 위한 컨텍스트 설정
		FAnimExtractContext ExtractContext;
		ExtractContext.CurrentTime = CurrentTime;
		ExtractContext.bLooping = bLooping;
		ExtractContext.Skeleton = Skeleton;

		// 5. 애니메이션 포즈 추출
		FPoseContext OutPoseContext;
		AnimSequence->GetAnimationPose(OutPoseContext, ExtractContext);

		// 6. 최종 포즈 저장
		FinalPose = OutPoseContext;
	}
}

void UAnimSingleNodeInstance::SetPlaying(bool bInIsPlaying)
{
	bIsPlaying = bInIsPlaying;
}

void UAnimSingleNodeInstance::SetLooping(bool bIsLooping)
{
	bLooping = bIsLooping;
}

void UAnimSingleNodeInstance::SetCurrentTime(float InTime)
{
	if (CurrentTime != InTime)
	{
		CurrentTime = InTime;
		bDirtyTime = true;
	}
}
