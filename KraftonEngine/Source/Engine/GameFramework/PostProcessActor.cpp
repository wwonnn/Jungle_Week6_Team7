#include "PostProcessActor.h"
#include "Component/FogComponent.h"
#include "Component/TextRenderComponent.h"
IMPLEMENT_CLASS(APostProcessActor, AActor)

void APostProcessActor::InitDefaultComponents()
{
	FogComponent = AddComponent<UFogComponent>();
	SetRootComponent(FogComponent);


	TextRenderComponent = AddComponent<UTextRenderComponent>();
	TextRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1.3f));
	TextRenderComponent->SetText("Height Fog");
	TextRenderComponent->AttachToComponent(FogComponent);
	TextRenderComponent->SetFont(FName("Default"));


}