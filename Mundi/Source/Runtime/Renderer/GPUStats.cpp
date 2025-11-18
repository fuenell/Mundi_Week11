#include "pch.h"
#include "GPUStats.h"
#include "D3D11RHI.h"

namespace
{
	static const FString GPrimitiveTotalKey("Primitives Total");
}

FGpuStatManager::~FGpuStatManager()
{
	ReleaseTimerQueries(CurrentFrameTimers);
	ReleaseTimerQueries(PreviousFrameTimers);
	ReleaseDisjointQuery(CurrentDisjointQuery);
	ReleaseDisjointQuery(PreviousDisjointQuery);
}

void FGpuStatManager::ResetFrameStats(D3D11RHI& RHI)
{
	PrimitiveRenderStats.Empty(); // 지난 프레임에 표시했던 통계 초기화
	ResolvePreviousFrameTimers(RHI);
	BeginNewFrameQuery(RHI);
}

void FGpuStatManager::EndFrame(D3D11RHI& RHI)
{
	if (bDisjointQueryBegun && CurrentDisjointQuery)
	{
		RHI.EndDisjointQuery(CurrentDisjointQuery);
		bDisjointQueryBegun = false;
	}

	PreviousFrameTimers = std::move(CurrentFrameTimers);
	CurrentFrameTimers.clear();

	ReleaseDisjointQuery(PreviousDisjointQuery);
	PreviousDisjointQuery = CurrentDisjointQuery;
	CurrentDisjointQuery = nullptr;
}

void FGpuStatManager::StartGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer)
{
	if (!bDisjointQueryBegun || !CurrentDisjointQuery)
	{
		return;
	}

	if (!Timer.TimestampStart)
	{
		if (FAILED(RHI.CreateTimestampQuery(&Timer.TimestampStart)))
		{
			Timer.TimestampStart = nullptr;
			return;
		}
	}

	RHI.WriteTimestamp(Timer.TimestampStart);
	Timer.bStartIssued = true;
}

void FGpuStatManager::FinishGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer)
{
	if (!Timer.bStartIssued || !bDisjointQueryBegun || !CurrentDisjointQuery)
	{
		return;
	}

	if (!Timer.TimestampEnd)
	{
		if (FAILED(RHI.CreateTimestampQuery(&Timer.TimestampEnd)))
		{
			return;
		}
	}

	RHI.WriteTimestamp(Timer.TimestampEnd);
	Timer.bEndIssued = true;

	CurrentFrameTimers.Add(Timer);

	Timer.TimestampStart = nullptr;
	Timer.TimestampEnd = nullptr;
	Timer.bStartIssued = false;
	Timer.bEndIssued = false;
}

const RenderStat* FGpuStatManager::FindPrimitiveStat(const FString& Key) const
{
	return PrimitiveRenderStats.Find(Key);
}

void FGpuStatManager::GetDisjointStats(uint64& OutFailCount, uint64& OutDisjointCount, uint64& OutJointCount) const
{
	OutFailCount = FailCount;
	OutDisjointCount = DisjointCount;
	OutJointCount = JointCount;
}

void FGpuStatManager::ResolvePreviousFrameTimers(D3D11RHI& RHI)
{
	// 이전 프레임에 측정된 타이머가 없으면 Disjoint 쿼리만 해제하고 종료
	if (PreviousFrameTimers.empty())
	{
		ReleaseDisjointQuery(PreviousDisjointQuery);
		return;
	}

	// 이전 프레임의 Disjoint 쿼리가 없으면 이전 프레임 타이머들은 전무 무효로 간주 -> 해제 후 종료
	if (!PreviousDisjointQuery)
	{
		++FailCount;
		ReleaseTimerQueries(PreviousFrameTimers);
		PreviousFrameTimers.clear();
		return;
	}

	// 이전 프레임 Disjoint 쿼리 결과가 유효하지 않거나 Disjoint 상태이면 이전 프레임 타이머들은 모두 무효로 간주 -> 해제 후 종료
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData{};
	bool bSuccess = RHI.GetDisjointQueryData(PreviousDisjointQuery, DisjointData, true);
	if (!bSuccess || DisjointData.Disjoint)
	{
		if(!bSuccess)
		{
			++FailCount;
		}
		else
		{
			++DisjointCount;
		}

		ReleaseTimerQueries(PreviousFrameTimers);
		PreviousFrameTimers.clear();
		ReleaseDisjointQuery(PreviousDisjointQuery);
		return;
	}

	++JointCount;

	// 각 타이머의 시작/끝 타임스탬프를 조회하여 GPU 시간 계산 및 누적
	for (FGpuTimer& Timer : PreviousFrameTimers)
	{
		if (!Timer.TimestampStart || !Timer.TimestampEnd)
		{
			continue;
		}

		UINT64 StartTimestamp = 0;
		UINT64 EndTimestamp = 0;
		const bool bStartReady = RHI.GetTimestampData(Timer.TimestampStart, StartTimestamp, true);
		const bool bEndReady = RHI.GetTimestampData(Timer.TimestampEnd, EndTimestamp, true);
		if (bStartReady && bEndReady)
		{
			const double Milliseconds = RHI.CalculateElapsedMilliseconds(StartTimestamp, EndTimestamp, DisjointData);
			if (Milliseconds >= 0.0)
			{
				AccumulateGpuStat(Timer.Type, Milliseconds);
			}
		}
	}

	ReleaseTimerQueries(PreviousFrameTimers);
	PreviousFrameTimers.clear();
	ReleaseDisjointQuery(PreviousDisjointQuery);
}

void FGpuStatManager::ReleaseDisjointQuery(ID3D11Query*& Query)
{
	if (Query)
	{
		Query->Release();
		Query = nullptr;
	}
}

void FGpuStatManager::AccumulateGpuStat(const FString& PrimitiveKey, double Milliseconds)
{
	RenderStat& Stat = PrimitiveRenderStats[PrimitiveKey];
	Stat.GpuRenderTimeMs += Milliseconds;
	Stat.CallCount++;

	if (PrimitiveKey != GPrimitiveTotalKey)
	{
		RenderStat& TotalStat = PrimitiveRenderStats[GPrimitiveTotalKey];
		TotalStat.GpuRenderTimeMs += Milliseconds;
		TotalStat.CallCount++;
	}
}

void FGpuStatManager::ReleaseTimerQueries(TArray<FGpuTimer>& Timers)
{
	for (FGpuTimer& Timer : Timers)
	{
		if (Timer.TimestampStart)
		{
			Timer.TimestampStart->Release();
			Timer.TimestampStart = nullptr;
		}
		if (Timer.TimestampEnd)
		{
			Timer.TimestampEnd->Release();
			Timer.TimestampEnd = nullptr;
		}
		Timer.bStartIssued = false;
		Timer.bEndIssued = false;
	}
}

void FGpuStatManager::BeginNewFrameQuery(D3D11RHI& RHI)
{
	ReleaseDisjointQuery(CurrentDisjointQuery);

	if (FAILED(RHI.CreateDisjointQuery(&CurrentDisjointQuery)) || !CurrentDisjointQuery)
	{
		UE_LOG("FGpuStatManager::BeginNewFrameQuery - Failed to create disjoint query.");
		CurrentDisjointQuery = nullptr;
		bDisjointQueryBegun = false;
		return;
	}

	RHI.BeginDisjointQuery(CurrentDisjointQuery);
	bDisjointQueryBegun = true;
}
