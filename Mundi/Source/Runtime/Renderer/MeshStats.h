#pragma once
#include <pch.h>

struct FGPUTimer
{
	ID3D11Query* TimestampStart = nullptr;
	ID3D11Query* TimestampEnd = nullptr;
	ID3D11Query* Disjoint = nullptr;

	double LastTimeMs = 0.0;
};
