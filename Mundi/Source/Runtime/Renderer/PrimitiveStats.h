#pragma once
#include <pch.h>
#include "PrimitiveTypeRegistry.h"
#include "PlatformTime.h"

struct FGPUTimer
{
	FString Type;
	ID3D11Query* TimestampStart = nullptr;
	ID3D11Query* TimestampEnd = nullptr;

	FGPUTimer(FString InType) : Type(InType) {}
};

struct RenderStat
{
	double CpuRenderTimeMs = 0.0;
	double GpuRenderTimeMs = 0.0;
	uint32 VertexCount = 0;

	RenderStat operator+=(const RenderStat& Other)
	{
		CpuRenderTimeMs += Other.CpuRenderTimeMs;
		GpuRenderTimeMs += Other.GpuRenderTimeMs;
		VertexCount += Other.VertexCount;
	}
};

class FPrimitiveStatManager
{
public:
	static FPrimitiveStatManager& GetInstance()
	{
		static FPrimitiveStatManager Instance;
		return Instance;
	}

	void ResetFrameStats(D3D11RHI& RHI);

	// 이전 프레임의 Disjoint 쿼리를 해제하고 해당 프레임의 Disjoint 쿼리를 새로 만듭니다.
	void NewGpuDisjointQuery(D3D11RHI& RHI);
	ID3D11Query* GetCurrentDisjointQuery() const { return CurrentDisjointQuery; }

	void StartGpuTimer(D3D11RHI& RHI, FGPUTimer& Timer);
	void FinishGpuTimer(D3D11RHI& RHI, FGPUTimer& Timer);
	void DestroyGPUTimer(FGPUTimer& Timer);

	bool IsDisjointQueryBegun() const { return bDisjointQueryBegun; }

private:
	// 싱글톤 패턴을 위해 생성/소멸자는 내부에서만 접근
	FPrimitiveStatManager() = default;
	~FPrimitiveStatManager() = default;
	// 싱글톤 패턴을 위해 복사 초기화 및 복사 대입 금지
	FPrimitiveStatManager(const FPrimitiveStatManager&) = delete;
	FPrimitiveStatManager& operator=(const FPrimitiveStatManager&) = delete;

private:
	ID3D11Query* CurrentDisjointQuery = nullptr;
	bool bDisjointQueryBegun = false;

	TMap<FString, RenderStat> PrimitiveRenderStats;
};
