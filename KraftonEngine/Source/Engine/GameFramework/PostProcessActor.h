#pragma once
#include "GameFramework/AActor.h"
class UTextRenderComponent;
class UFogComponent;
class APostProcessActor : public AActor
{
public:
	DECLARE_CLASS(APostProcessActor, AActor)

	APostProcessActor() = default;

    void InitDefaultComponents();
	UFogComponent* FogComponent = nullptr;
	UTextRenderComponent* TextRenderComponent = nullptr;
};