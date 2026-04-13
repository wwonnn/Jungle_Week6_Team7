#pragma once

#include "GameFramework/AActor.h"

class UHeightFogComponent;

class AExponentialHeightFogActor : public AActor
{
public:
	DECLARE_CLASS(AExponentialHeightFogActor, AActor)
	AExponentialHeightFogActor() {}

	void InitDefaultComponents();

	UHeightFogComponent* GetFogComponent() const { return FogComponent; }

private:
	UHeightFogComponent* FogComponent = nullptr;
};
