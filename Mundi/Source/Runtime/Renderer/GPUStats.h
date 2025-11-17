#pragma once
#include <pch.h>
#include "PrimitiveTypeRegistry.h"
#include "PlatformTime.h"

struct FGpuTimer
{
	FString Type;
	ID3D11Query* TimestampStart = nullptr;
	ID3D11Query* TimestampEnd = nullptr;

	FGpuTimer(FString InType) : Type(InType) {}
};

struct RenderStat
{
	double CpuRenderTimeMs = 0.0;
	double GpuRenderTimeMs = 0.0;
	// uint32 VertexCount = 0; 버텍스 카운트는 나중에 필요하면 추가

	RenderStat operator+=(const RenderStat& Other)
	{
		CpuRenderTimeMs += Other.CpuRenderTimeMs;
		GpuRenderTimeMs += Other.GpuRenderTimeMs;
		// VertexCount += Other.VertexCount;
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

	// 이전 프레임의 Disjoint 쿼리를 해제하고 해당 프레임의 Disjoint 쿼리를 새로 만듭니다.
	void NewGpuDisjointQuery(D3D11RHI& RHI);
	ID3D11Query* GetCurrentDisjointQuery() const { return CurrentDisjointQuery; }

	void StartGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer);
	void FinishGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer);
	void DestroyGpuTimer(FGpuTimer& Timer);

	bool IsDisjointQueryBegun() const { return bDisjointQueryBegun; }

private:
	// 싱글톤 패턴을 위해 생성/소멸자는 내부에서만 접근
	FGpuStatManager() = default;
	~FGpuStatManager() = default;
	// 싱글톤 패턴을 위해 복사 초기화 및 복사 대입 금지
	FGpuStatManager(const FGpuStatManager&) = delete;
	FGpuStatManager& operator=(const FGpuStatManager&) = delete;

private:
	ID3D11Query* CurrentDisjointQuery = nullptr;
	bool bDisjointQueryBegun = false;

	TMap<FString, RenderStat> PrimitiveRenderStats;
};
