#pragma once
#include "GameFramework/AActor.h"
class UTextRenderComponent;
class UExponentialHeightFogComponent;
class AExponentialHeightFog : public AActor
{
public:
	DECLARE_CLASS(AExponentialHeightFog, AActor)

	AExponentialHeightFog() = default;

    void InitDefaultComponents();
	UExponentialHeightFogComponent* FogComponent = nullptr;
	UTextRenderComponent* TextRenderComponent = nullptr;
};