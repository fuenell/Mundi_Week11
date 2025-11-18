#pragma once
#include <pch.h>
#include "PrimitiveTypeRegistry.h"
#include "PlatformTime.h"

class D3D11RHI;

struct FGpuTimer
{
	FString Type;
	ID3D11Query* TimestampStart = nullptr;
	ID3D11Query* TimestampEnd = nullptr;
	bool bStartIssued = false;
	bool bEndIssued = false;

	explicit FGpuTimer(FString InType) : Type(std::move(InType)) {}
};

struct RenderStat
{
	double CpuRenderTimeMs = 0.0;
	double GpuRenderTimeMs = 0.0;
	uint32 CallCount = 0;

	RenderStat& operator+=(const RenderStat& Other)
	{
		CpuRenderTimeMs += Other.CpuRenderTimeMs;
		GpuRenderTimeMs += Other.GpuRenderTimeMs;
		CallCount += Other.CallCount;
		return *this;
	}
};

class FGpuStatManager
{
public:
	static FGpuStatManager& GetInstance()
	{
		static FGpuStatManager Instance;
		return Instance;
	}

	void ResetFrameStats(D3D11RHI& RHI);
	void EndFrame(D3D11RHI& RHI);

	void StartGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer);
	void FinishGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer);

	const RenderStat* FindPrimitiveStat(const FString& Key) const;
	const TMap<FString, RenderStat>& GetPrimitiveRenderStats() const { return PrimitiveRenderStats; }
	void GetDisjointStats(uint64& OutFailCount, uint64& OutDisjointCount, uint64& OutJointCount) const;

private:
	FGpuStatManager() = default;
	~FGpuStatManager();
	FGpuStatManager(const FGpuStatManager&) = delete;
	FGpuStatManager& operator=(const FGpuStatManager&) = delete;

	void ResolvePreviousFrameTimers(D3D11RHI& RHI);
	void ReleaseDisjointQuery(ID3D11Query*& Query);
	void AccumulateGpuStat(const FString& PrimitiveKey, double Milliseconds);
	void ReleaseTimerQueries(TArray<FGpuTimer>& Timers);
	void BeginNewFrameQuery(D3D11RHI& RHI);

private:
	ID3D11Query* CurrentDisjointQuery = nullptr;
	ID3D11Query* PreviousDisjointQuery = nullptr;
	bool bDisjointQueryBegun = false;

	TArray<FGpuTimer> CurrentFrameTimers;
	TArray<FGpuTimer> PreviousFrameTimers;
	TMap<FString, RenderStat> PrimitiveRenderStats;

	uint64 FailCount = 0;
	uint64 DisjointCount = 0;
	uint64 JointCount = 0;
};
