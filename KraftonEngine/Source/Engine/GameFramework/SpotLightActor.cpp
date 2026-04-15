#include "SpotLightActor.h"
#include "Object/ObjectFactory.h"
#include "Component/SpotLightDecalComponent.h"
#include "Component/BillboardComponent.h"

IMPLEMENT_CLASS(ASpotLightActor, AActor)

void ASpotLightActor::InitDefaultComponents()
{
	LightComponent = AddComponent<USpotLightDecalComponent>();
	SetRootComponent(LightComponent);

	// 에디터에서 아이콘으로 표시할 Billboard
	SpriteComponent = AddComponent<UBillboardComponent>();
	SpriteComponent->SetTexture("SpotLight");
	SpriteComponent->AttachToComponent(LightComponent);
}