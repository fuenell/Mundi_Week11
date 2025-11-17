#include "pch.h"
#include "PrimitiveStats.h"

void FPrimitiveStatManager::ResetFrameStats(D3D11RHI& RHI)
{
	NewGpuDisjointQuery(RHI);

	PrimitiveRenderStats.Empty();
}

void FPrimitiveStatManager::NewGpuDisjointQuery(D3D11RHI& RHI)
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

void FPrimitiveStatManager::StartGpuTimer(D3D11RHI& RHI, FGPUTimer& Timer)
{

}

void FPrimitiveStatManager::FinishGpuTimer(D3D11RHI& RHI, FGPUTimer& Timer)
{

}

void FPrimitiveStatManager::DestroyGPUTimer(FGPUTimer& Timer)
{

}
