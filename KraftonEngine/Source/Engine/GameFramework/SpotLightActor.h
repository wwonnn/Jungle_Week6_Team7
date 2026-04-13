#pragma once

#include "GameFramework/AActor.h"

class UDecalComponent;
class UBillboardComponent;

class ASpotLightActor : public AActor
{
public:
	DECLARE_CLASS(ASpotLightActor, AActor)
	ASpotLightActor() {}

	void InitDefaultComponents();

private:
	UBillboardComponent* LightSource = nullptr;
	UDecalComponent* FloorDecal = nullptr;
};
