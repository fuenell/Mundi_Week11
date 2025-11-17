#include "pch.h"
#include "GPUStats.h"

void FGpuStatManager::ResetFrameStats(D3D11RHI& RHI)
{
	NewGpuDisjointQuery(RHI);

	PrimitiveRenderStats.Empty();
}

void FGpuStatManager::NewGpuDisjointQuery(D3D11RHI& RHI)
{
	// 이전 쿼리 해제
	if (CurrentDisjointQuery)
	{
		if (bDisjointQueryBegun)
		{
			RHI.GetDeviceContext()->End(CurrentDisjointQuery);
			bDisjointQueryBegun = false;
		}
		CurrentDisjointQuery->Release();
		CurrentDisjointQuery = nullptr;
	}

	// 새 쿼리 생성
	D3D11_QUERY_DESC QueryDesc{};
	QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	HRESULT hr = RHI.CreateDisjointQuery(&CurrentDisjointQuery);
	if (FAILED(hr))
	{
		UE_LOG("FPrimitiveStatManager::NewGpuDisjointQuery - Failed to create disjoint query.");
		CurrentDisjointQuery = nullptr;
		return;
	}

	// 새 쿼리 시작
	RHI.GetDeviceContext()->Begin(CurrentDisjointQuery);
	bDisjointQueryBegun = true;
	CurrentDisjointQuery = CurrentDisjointQuery;
}

void FGpuStatManager::StartGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer)
{

}

void FGpuStatManager::FinishGpuTimer(D3D11RHI& RHI, FGpuTimer& Timer)
{

}

void FGpuStatManager::DestroyGpuTimer(FGpuTimer& Timer)
{

}
