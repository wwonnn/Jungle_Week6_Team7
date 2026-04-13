#include "ExponentialHeightFogActor.h"
#include "Object/ObjectFactory.h"
#include "Component/HeightFogComponent.h"

IMPLEMENT_CLASS(AExponentialHeightFogActor, AActor)

void AExponentialHeightFogActor::InitDefaultComponents()
{
	FogComponent = AddComponent<UHeightFogComponent>();
	SetRootComponent(FogComponent);
}
