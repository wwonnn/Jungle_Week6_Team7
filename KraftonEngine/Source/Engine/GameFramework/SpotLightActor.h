#pragma once
#include "GameFramework/AActor.h"

class USpotLightDecalComponent;
class UBillboardComponent;

class ASpotLightActor : public AActor
{
public:
	DECLARE_CLASS(ASpotLightActor, AActor)

	ASpotLightActor() {}

	void InitDefaultComponents();

	USpotLightDecalComponent* GetLightComponent() const { return LightComponent; }

private:
	USpotLightDecalComponent* LightComponent = nullptr;
	UBillboardComponent* SpriteComponent = nullptr;
};