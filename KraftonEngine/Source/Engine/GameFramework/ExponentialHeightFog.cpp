#include "ExponentialHeightFog.h"
#include "Component/ExponentialHeightFogComponent.h"
#include "Component/TextRenderComponent.h"
IMPLEMENT_CLASS(AExponentialHeightFog, AActor)

void AExponentialHeightFog::InitDefaultComponents()
{
	FogComponent = AddComponent<UExponentialHeightFogComponent>();
	SetRootComponent(FogComponent);


	TextRenderComponent = AddComponent<UTextRenderComponent>();
	TextRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1.3f));
	TextRenderComponent->SetText("Height Fog");
	TextRenderComponent->AttachToComponent(FogComponent);
	TextRenderComponent->SetFont(FName("Default"));


}