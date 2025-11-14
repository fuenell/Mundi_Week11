#pragma once
#include "Vector.h"
#include "Picking.h"

enum class EAxis : uint8
{
	X = 0,
	Y = 1,
	Z = 2
};

// NOTE:
// Originally named FBound.
// Renamed to FAABB as part of introducing OBB support (2025-10).
// This struct now explicitly represents an Axis-Aligned Bounding Box.
struct FAABB
{
	FVector Min;
	FVector Max;

	FAABB();
	FAABB(const FVector& InMin, const FVector& InMax);
	FAABB(const FVector* Vertices, uint32 Size);
	FAABB(const TArray<FVector>& Vertices);

	// 중심점
	FVector GetCenter() const;

	// 반쪽 크기 (Extent)
	FVector GetHalfExtent() const;

	TArray<FVector> GetVertices() const;


	// 다른 박스를 완전히 포함하는지 확인
	bool Contains(const FAABB& Other) const;

	// 교차 여부만 확인
	bool Intersects(const FAABB& Other) const;

	// i번째 옥탄트 Bounds 반환
	FAABB CreateOctant(int i) const;
	
	void SnapToGrid(const FVector& GridSize, bool bFloor);

	bool IntersectsRay(const FRay& InRay, float& OutEnterDistance, float& OutExitDistance);

	static FAABB Union(const FAABB& A, const FAABB& B);

};

inline TArray<FVector> CubeVerticesToLine(const TArray<FVector>& CubeVertices)
{
	TArray<FVector> Lines;
	if (CubeVertices.Num() % 8 != 0)
	{
		return Lines;
	}
	int32 CubeCount = CubeVertices.Num() / 8;
	Lines.reserve(24 * CubeCount);

	for (int i = 0; i < CubeCount; i++)
	{
		Lines.emplace_back(CubeVertices[i * 8 + 0]);
		Lines.emplace_back(CubeVertices[i * 8 + 1]);
		Lines.emplace_back(CubeVertices[i * 8 + 1]);
		Lines.emplace_back(CubeVertices[i * 8 + 2]);
		Lines.emplace_back(CubeVertices[i * 8 + 2]);
		Lines.emplace_back(CubeVertices[i * 8 + 3]);
		Lines.emplace_back(CubeVertices[i * 8 + 3]);
		Lines.emplace_back(CubeVertices[i * 8 + 0]);

		Lines.emplace_back(CubeVertices[i * 8 + 0]);
		Lines.emplace_back(CubeVertices[i * 8 + 4]);
		Lines.emplace_back(CubeVertices[i * 8 + 1]);
		Lines.emplace_back(CubeVertices[i * 8 + 5]);
		Lines.emplace_back(CubeVertices[i * 8 + 2]);
		Lines.emplace_back(CubeVertices[i * 8 + 6]);
		Lines.emplace_back(CubeVertices[i * 8 + 3]);
		Lines.emplace_back(CubeVertices[i * 8 + 7]);

		Lines.emplace_back(CubeVertices[i * 8 + 4]);
		Lines.emplace_back(CubeVertices[i * 8 + 5]);
		Lines.emplace_back(CubeVertices[i * 8 + 5]);
		Lines.emplace_back(CubeVertices[i * 8 + 6]);
		Lines.emplace_back(CubeVertices[i * 8 + 6]);
		Lines.emplace_back(CubeVertices[i * 8 + 7]);
		Lines.emplace_back(CubeVertices[i * 8 + 7]);
		Lines.emplace_back(CubeVertices[i * 8 + 4]);
	}
	return Lines;
}