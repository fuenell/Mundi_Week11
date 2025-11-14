#include "pch.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>

#include "StatsOverlayD2D.h"
#include "UIManager.h"
#include "MemoryManager.h"
#include "Picking.h"
#include "PlatformTime.h"
#include "DecalStatManager.h"
#include "TileCullingStats.h"
#include "LightStats.h"
#include "ShadowStats.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

template <typename T>
static inline void SafeRelease(T*& Ptr)
{
	if (Ptr)
	{
		Ptr->Release();
		Ptr = nullptr;
	}
}

UStatsOverlayD2D& UStatsOverlayD2D::Get()
{
	static UStatsOverlayD2D Instance;
	return Instance;
}

void UStatsOverlayD2D::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext, IDXGISwapChain* InSwapChain)
{
	ReleaseD2DResources();

	D3DDevice = InDevice;
	D3DContext = InContext;
	SwapChain = InSwapChain;
	bInitialized = (D3DDevice && D3DContext && SwapChain);

	if (!bInitialized)
	{
		return;
	}

	D2D1_FACTORY_OPTIONS FactoryOpts{};
#ifdef _DEBUG
	FactoryOpts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &FactoryOpts, (void**)&D2DFactory)))
	{
		ReleaseD2DResources();
		bInitialized = false;
		return;
	}

	IDXGIDevice* DxgiDevice = nullptr;
	if (FAILED(D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DxgiDevice)))
	{
		ReleaseD2DResources();
		bInitialized = false;
		return;
	}

	if (FAILED(D2DFactory->CreateDevice(DxgiDevice, &D2DDevice)))
	{
		SafeRelease(DxgiDevice);
		ReleaseD2DResources();
		bInitialized = false;
		return;
	}
	SafeRelease(DxgiDevice);

	if (FAILED(D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2DContext)))
	{
		ReleaseD2DResources();
		bInitialized = false;
		return;
	}

	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&DWriteFactory)))
	{
		ReleaseD2DResources();
		bInitialized = false;
		return;
	}

	if (DWriteFactory)
	{
		DWriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			16.0f,
			L"en-us",
			&TextFormat);

		if (TextFormat)
		{
			TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}
	}

	EnsureInitialized();
}

void UStatsOverlayD2D::Shutdown()
{
	ReleaseD2DResources();

	D3DDevice = nullptr;
	D3DContext = nullptr;
	SwapChain = nullptr;
	bInitialized = false;
}

void UStatsOverlayD2D::EnsureInitialized()
{
	if (!D2DContext)
	{
		return;
	}

	SafeRelease(BrushYellow);
	SafeRelease(BrushSkyBlue);
	SafeRelease(BrushLightGreen);
	SafeRelease(BrushOrange);
	SafeRelease(BrushCyan);
	SafeRelease(BrushViolet);
	SafeRelease(BrushDeepPink);
	SafeRelease(BrushBlack);

	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &BrushYellow);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::SkyBlue), &BrushSkyBlue);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGreen), &BrushLightGreen);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Orange), &BrushOrange);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Cyan), &BrushCyan);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Violet), &BrushViolet);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DeepPink), &BrushDeepPink);
	D2DContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.6f), &BrushBlack);
}

void UStatsOverlayD2D::ReleaseD2DResources()
{
	SafeRelease(BrushBlack);
	SafeRelease(BrushDeepPink);
	SafeRelease(BrushViolet);
	SafeRelease(BrushCyan);
	SafeRelease(BrushOrange);
	SafeRelease(BrushLightGreen);
	SafeRelease(BrushSkyBlue);
	SafeRelease(BrushYellow);

	SafeRelease(TextFormat);
	SafeRelease(DWriteFactory);
	SafeRelease(D2DContext);
	SafeRelease(D2DDevice);
	SafeRelease(D2DFactory);
}

static void DrawTextBlock(
	ID2D1DeviceContext* InD2dCtx,
	IDWriteTextFormat* InTextFormat,
	const wchar_t* InText,
	const D2D1_RECT_F& InRect,
	ID2D1SolidColorBrush* InBgBrush,
	ID2D1SolidColorBrush* InTextBrush)
{
	if (!InD2dCtx || !InTextFormat || !InText || !InBgBrush || !InTextBrush)
	{
		return;
	}

	InD2dCtx->FillRectangle(InRect, InBgBrush);
	InD2dCtx->DrawTextW(
		InText,
		static_cast<UINT32>(wcslen(InText)),
		InTextFormat,
		InRect,
		InTextBrush);
}

void UStatsOverlayD2D::Draw()
{
	if (!bInitialized || (!bShowFPS && !bShowMemory && !bShowPicking && !bShowDecal && !bShowTileCulling && !bShowLights && !bShowShadow) || !SwapChain)
	{
		return;
	}

	if (!D2DContext || !TextFormat)
	{
		return;
	}

	IDXGISurface* Surface = nullptr;
	if (FAILED(SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&Surface)))
	{
		return;
	}

	D2D1_BITMAP_PROPERTIES1 BmpProps{};
	BmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	BmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	BmpProps.dpiX = 96.0f;
	BmpProps.dpiY = 96.0f;
	BmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

	ID2D1Bitmap1* TargetBmp = nullptr;
	if (FAILED(D2DContext->CreateBitmapFromDxgiSurface(Surface, &BmpProps, &TargetBmp)))
	{
		SafeRelease(Surface);
		return;
	}

	D2DContext->SetTarget(TargetBmp);
	D2DContext->BeginDraw();

	const float Margin = 12.0f;
	const float Space = 8.0f;
	const float PanelWidth = 200.0f;
	const float PanelHeight = 48.0f;
	float NextY = 70.0f;

	if (bShowFPS)
	{
		float Dt = UUIManager::GetInstance().GetDeltaTime();
		float Fps = Dt > 0.0f ? (1.0f / Dt) : 0.0f;
		float Ms = Dt * 1000.0f;

		wchar_t Buf[128];
		swprintf_s(Buf, L"FPS: %.1f\nFrame time: %.2f ms", Fps, Ms);

		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, rc, BrushBlack, BrushYellow);

		NextY += PanelHeight + Space;
	}

	if (bShowPicking)
	{
		wchar_t Buf[256];
		double LastMs = FWindowsPlatformTime::ToMilliseconds(CPickingSystem::GetLastPickTime());
		double TotalMs = FWindowsPlatformTime::ToMilliseconds(CPickingSystem::GetTotalPickTime());
		uint32_t Count = CPickingSystem::GetPickCount();
		double AvgMs = (Count > 0) ? (TotalMs / (double)Count) : 0.0;
		swprintf_s(Buf, L"Pick Count: %u\nLast: %.3f ms\nAvg: %.3f ms\nTotal: %.3f ms", Count, LastMs, AvgMs, TotalMs);

		const float PickPanelHeight = 96.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PickPanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, rc, BrushBlack, BrushSkyBlue);

		NextY += PickPanelHeight + Space;
	}

	if (bShowMemory)
	{
		double Bytes = (double)FMemoryManager::TotalAllocationBytes;
		double Mb = Bytes / (1024.0 * 1024.0);

		wchar_t Buf[128];
		swprintf_s(Buf, L"Memory: %.1f MB\nAllocs: %u", Mb, FMemoryManager::TotalAllocationCount);

		D2D1_RECT_F Rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, Rc, BrushBlack, BrushLightGreen);

		NextY += PanelHeight + Space;
	}

	if (bShowDecal)
	{
		// 1. FDecalStatManager로부터 통계 데이터를 가져옵니다.
		uint32_t TotalCount = FDecalStatManager::GetInstance().GetTotalDecalCount();
		//uint32_t VisibleDecalCount = FDecalStatManager::GetInstance().GetVisibleDecalCount();
		uint32_t AffectedMeshCount = FDecalStatManager::GetInstance().GetAffectedMeshCount();
		double TotalTime = FDecalStatManager::GetInstance().GetDecalPassTimeMS();
		double AverageTimePerDecal = FDecalStatManager::GetInstance().GetAverageTimePerDecalMS();
		double AverageTimePerDraw = FDecalStatManager::GetInstance().GetAverageTimePerDrawMS();

		// 2. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Decal Stats]\nTotal: %u\nAffectedMesh: %u\n전체 소요 시간: %.3f ms\nAvg/Decal: %.3f ms\nAvg/Mesh: %.3f ms",
			TotalCount,
			AffectedMeshCount,
			TotalTime,
			AverageTimePerDecal,
			AverageTimePerDraw);

		const float decalPanelHeight = 140.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + decalPanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, rc, BrushBlack, BrushOrange);

		NextY += decalPanelHeight + Space;
	}

	if (bShowTileCulling)
	{
		const FTileCullingStats& TileStats = FTileCullingStatManager::GetInstance().GetStats();

		wchar_t Buf[512];
		swprintf_s(Buf, L"[Tile Culling Stats]\nTiles: %u x %u (%u)\nLights: %u (P:%u S:%u)\nMin/Avg/Max: %u / %.1f / %u\nCulling Eff: %.1f%%\nBuffer: %u KB",
			TileStats.TileCountX,
			TileStats.TileCountY,
			TileStats.TotalTileCount,
			TileStats.TotalLights,
			TileStats.TotalPointLights,
			TileStats.TotalSpotLights,
			TileStats.MinLightsPerTile,
			TileStats.AvgLightsPerTile,
			TileStats.MaxLightsPerTile,
			TileStats.CullingEfficiency,
			TileStats.LightIndexBufferSizeBytes / 1024);

		const float tilePanelHeight = 160.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + tilePanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, rc, BrushBlack, BrushCyan);

		NextY += tilePanelHeight + Space;
	}

	if (bShowLights)
	{
		const FLightStats& LightStats = FLightStatManager::GetInstance().GetStats();

		wchar_t Buf[512];
		swprintf_s(Buf, L"[Light Stats]\nTotal Lights: %u\n  Point: %u\n  Spot: %u\n  Directional: %u\n  Ambient: %u",
			LightStats.TotalLights,
			LightStats.TotalPointLights,
			LightStats.TotalSpotLights,
			LightStats.TotalDirectionalLights,
			LightStats.TotalAmbientLights);

		const float lightPanelHeight = 140.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + lightPanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, rc, BrushBlack, BrushViolet);

		NextY += lightPanelHeight + Space;
	}

	if (bShowShadow)
	{
		const FShadowStats& ShadowStats = FShadowStatManager::GetInstance().GetStats();

		wchar_t Buf[512];
		swprintf_s(Buf, L"[Shadow Stats]\nShadow Lights: %u\n  Point: %u\n  Spot: %u\n  Directional: %u\n\nAtlas 2D: %u x %u (%.1f MB)\nAtlas Cube: %u x %u x %u (%.1f MB)\n\nTotal Memory: %.1f MB",
			ShadowStats.TotalShadowCastingLights,
			ShadowStats.ShadowCastingPointLights,
			ShadowStats.ShadowCastingSpotLights,
			ShadowStats.ShadowCastingDirectionalLights,
			ShadowStats.ShadowAtlas2DSize,
			ShadowStats.ShadowAtlas2DSize,
			ShadowStats.ShadowAtlas2DMemoryMB,
			ShadowStats.ShadowAtlasCubeSize,
			ShadowStats.ShadowAtlasCubeSize,
			ShadowStats.ShadowCubeArrayCount,
			ShadowStats.ShadowAtlasCubeMemoryMB,
			ShadowStats.TotalShadowMemoryMB);

		const float shadowPanelHeight = 260.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + shadowPanelHeight);
		DrawTextBlock(D2DContext, TextFormat, Buf, rc, BrushBlack, BrushDeepPink);

		NextY += shadowPanelHeight + Space;

		rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + 40);
		DrawTextBlock(D2DContext, TextFormat, FScopeCycleCounter::GetTimeProfile("ShadowMapPass").GetConstWChar_tWithKey("ShadowMapPass"), rc, BrushBlack, BrushDeepPink);

		NextY += shadowPanelHeight + Space;
	}

	D2DContext->EndDraw();
	D2DContext->SetTarget(nullptr);

	FScopeCycleCounter::TimeProfileInit();

	SafeRelease(TargetBmp);
	SafeRelease(Surface);
}


void UStatsOverlayD2D::SetShowFPS(bool b)
{
	bShowFPS = b;
}

void UStatsOverlayD2D::SetShowMemory(bool b)
{
	bShowMemory = b;
}

void UStatsOverlayD2D::SetShowPicking(bool b)
{
	bShowPicking = b;
}

void UStatsOverlayD2D::SetShowDecal(bool b)
{
	bShowDecal = b;
}

void UStatsOverlayD2D::ToggleFPS()
{
	bShowFPS = !bShowFPS;
}

void UStatsOverlayD2D::ToggleMemory()
{
	bShowMemory = !bShowMemory;
}

void UStatsOverlayD2D::TogglePicking()
{
	bShowPicking = !bShowPicking;
}

void UStatsOverlayD2D::ToggleDecal()
{
	bShowDecal = !bShowDecal;
}

void UStatsOverlayD2D::SetShowTileCulling(bool b)
{
	bShowTileCulling = b;
}

void UStatsOverlayD2D::ToggleTileCulling()
{
	bShowTileCulling = !bShowTileCulling;
}

void UStatsOverlayD2D::SetShowLights(bool b)
{
	bShowLights = b;
}

void UStatsOverlayD2D::ToggleLights()
{
	bShowLights = !bShowLights;
}

void UStatsOverlayD2D::SetShowShadow(bool b)
{
	bShowShadow = b;
}

void UStatsOverlayD2D::ToggleShadow()
{
	bShowShadow = !bShowShadow;
}
