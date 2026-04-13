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
// 산란광(Light Shaft)을 표현할 추가 빌보드
// LightShaft = AddComponent<UBillboardComponent>();
// LightShaft->AttachToComponent(LightSource);
// 산란광용 텍스처는 일단 비워둠 (적절한 에셋 획득 후 설정)
// LightShaft->SetTexture("LightShaftTextureName"); 
// 살짝 아래로 내려서 빛이 퍼져나가는 느낌을 줌
// LightShaft->SetRelativeLocation(FVector(0.f, 0.f, -0.5f));

// 바닥에 맺히는 빛
FloorDecal = AddComponent<UDecalComponent>();
FloorDecal->AttachToComponent(LightSource);
FloorDecal->SetTexture("Floor_Light"); 
FloorDecal->SetRelativeRotation(FVector(0.f, 90.f, 0.f));    

// 거리가 멀어지거나 가장자리로 갈수록 부드럽게 사라지는 페이드(Fade) 효과 활성화
// Inner(0.2)부터 서서히 흐려지기 시작해서 Outer(0.8)에서 완전히 투명해짐
FloorDecal->SetFade(true, 0.2f, 0.8f);

// 빛이 맺히는 바닥의 위치 (절댓값 1 활용)
FloorDecal->SetRelativeLocation(FVector(0.f, 0.f, -1.f));
FloorDecal->SetRelativeScale(FVector(2.f, 1.f, 1.f));
}
