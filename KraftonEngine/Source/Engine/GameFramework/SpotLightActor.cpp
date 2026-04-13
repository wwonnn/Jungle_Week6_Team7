#include "SpotLightActor.h"
#include "Component/DecalComponent.h"
#include "Component/BillboardComponent.h"

IMPLEMENT_CLASS(ASpotLightActor, AActor)

void ASpotLightActor::InitDefaultComponents()
{
	// 빛의 근원지
	LightSource = AddComponent<UBillboardComponent>();
	SetRootComponent(LightSource);
	LightSource->SetTexture("SpotLight");

	// 바닥에 맺히는 빛
	FloorDecal = AddComponent<UDecalComponent>();
	FloorDecal->AttachToComponent(LightSource);
	
	// 임시 텍스처 (나중에 부드러운 원형 그라데이션 텍스처로 변경)
	FloorDecal->SetTexture("Pawn"); 
	FloorDecal->SetRelativeLocation(FVector(0.f, 0.f, -1.f));
}
