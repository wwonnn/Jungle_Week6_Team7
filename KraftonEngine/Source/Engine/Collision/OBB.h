#pragma once
#include "Math/Vector.h"
#include "Core/EngineTypes.h"
struct FOBB
{
	FVector Center;
	FVector Axes[3];
	FVector HalfExtent;

	static FOBB FromAABB(const FBoundingBox& AABB)
	{
		FOBB Result;
		Result.Center = AABB.GetCenter();
		Result.Axes[0] = FVector(1, 0, 0);
		Result.Axes[1] = FVector(0, 1, 0);
		Result.Axes[2] = FVector(0, 0, 1);
		Result.HalfExtent = AABB.GetExtent();
		return Result;
	}
};
