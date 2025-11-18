#include "pch.h"
#include "GPUStats.h"
#include "D3D11RHI.h"

namespace
{
	static const FString GPrimitiveTotalKey("Primitives Total");
}

FGpuStatManager::FGpuStatManager()
{
	FrameBatches.resize(BufferedFrameCount);
}

FGpuStatManager::~FGpuStatManager()
{
	for (FFrameTimingBatch& Batch : FrameBatches)
	{
		ReleaseTimerQueries(Batch.Timers);
		ReleaseDisjointQuery(Batch.DisjointQuery);
		Batch.Timers.clear();
		Batch.bDisjointQueryBegun = false;
		Batch.bPendingResolve = false;
	}
}

void FGpuStatManager::ResetFrameStats(D3D11RHI& RHI)
{
	ResolveFrameBatch(RHI, CurrentBatchIndex);
	PrepareFrameBatch(CurrentBatchIndex);
	BeginNewFrameQuery(RHI);
}

void FGpuStatManager::EndFrame(D3D11RHI& RHI)
{
	if (FrameBatches.empty())
	{
		return;
	}

	FFrameTimingBatch& Batch = FrameBatches[CurrentBatchIndex];
	if (Batch.bDisjointQueryBegun && Batch.DisjointQuery)
	{
		RHI.EndDisjointQuery(Batch.DisjointQuery);
		Batch.bDisjointQueryBegun = false;
	}

	Batch.bPendingResolve = true;
	ActiveBatch = nullptr;

	CurrentBatchIndex = (CurrentBatchIndex + 1) % BufferedFrameCount;
}

void FGpuStatManager::StartGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer)
{
	if (!ActiveBatch || !ActiveBatch->bDisjointQueryBegun || !ActiveBatch->DisjointQuery)
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
	if (!ActiveBatch || !Timer.bStartIssued || !ActiveBatch->bDisjointQueryBegun || !ActiveBatch->DisjointQuery)
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

	ActiveBatch->Timers.Add(Timer);

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

void FGpuStatManager::ResolveFrameBatch(D3D11RHI& RHI, uint32 BatchIndex)
{
	if (FrameBatches.empty())
	{
		return;
	}

	FFrameTimingBatch& Batch = FrameBatches[BatchIndex];
	if (!Batch.bPendingResolve)
	{
		return;
	}

	PrimitiveRenderStats.Empty();

	if (!Batch.DisjointQuery)
	{
		++FailCount;
		ReleaseTimerQueries(Batch.Timers);
		Batch.Timers.clear();
		Batch.bPendingResolve = false;
		return;
	}

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData{};
	const bool bSuccess = RHI.GetDisjointQueryData(Batch.DisjointQuery, DisjointData, true);
	if (!bSuccess || DisjointData.Disjoint)
	{
		/*if (!bSuccess)
		{
			++FailCount;
		}
		else
		{
			++DisjointCount;
		}*/

		ReleaseTimerQueries(Batch.Timers);
		Batch.Timers.clear();
		ReleaseDisjointQuery(Batch.DisjointQuery);
		Batch.bPendingResolve = false;
		return;
	}

	// ++JointCount;

	for (FGpuTimer& Timer : Batch.Timers)
	{
		if (!Timer.TimestampStart || !Timer.TimestampEnd)
		{
			continue;
		}

		UINT64 StartTimestamp = 0;
		UINT64 EndTimestamp = 0;
		const bool bStartReady = RHI.GetTimestampData(Timer.TimestampStart, StartTimestamp, true);
		const bool bEndReady = RHI.GetTimestampData(Timer.TimestampEnd, EndTimestamp, true);
		if (!bStartReady || !bEndReady)
		{
			continue;
		}

		const double Milliseconds = RHI.CalculateElapsedMilliseconds(StartTimestamp, EndTimestamp, DisjointData);
		if (Milliseconds >= 0.0)
		{
			AccumulateGpuStat(Timer.Type, Milliseconds);
		}
	}

	ReleaseTimerQueries(Batch.Timers);
	Batch.Timers.clear();
	ReleaseDisjointQuery(Batch.DisjointQuery);
	Batch.bDisjointQueryBegun = false;
	Batch.bPendingResolve = false;
}

void FGpuStatManager::PrepareFrameBatch(uint32 BatchIndex)
{
	if (FrameBatches.empty())
	{
		FrameBatches.resize(BufferedFrameCount);
	}

	ActiveBatch = &FrameBatches[BatchIndex];
	FFrameTimingBatch& Batch = *ActiveBatch;

	ReleaseTimerQueries(Batch.Timers);
	Batch.Timers.clear();
	ReleaseDisjointQuery(Batch.DisjointQuery);
	Batch.bDisjointQueryBegun = false;
	Batch.bPendingResolve = false;
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
	if (FrameBatches.empty())
	{
		FrameBatches.resize(BufferedFrameCount);
	}

	FFrameTimingBatch& Batch = FrameBatches[CurrentBatchIndex];
	ActiveBatch = &Batch;

	if (FAILED(RHI.CreateDisjointQuery(&Batch.DisjointQuery)) || !Batch.DisjointQuery)
	{
		UE_LOG("FGpuStatManager::BeginNewFrameQuery - Failed to create disjoint query.");
		Batch.DisjointQuery = nullptr;
		Batch.bDisjointQueryBegun = false;
		return;
	}

	RHI.BeginDisjointQuery(Batch.DisjointQuery);
	Batch.bDisjointQueryBegun = true;
}
