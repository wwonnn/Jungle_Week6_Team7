#include "FireBallActor.h"
#include "Component/StaticMeshComponent.h"
#include "Component/DecalComponent.h"
#include "Component/BillboardComponent.h"
#include "Mesh/ObjManager.h"
#include "Engine/Runtime/Engine.h"
IMPLEMENT_CLASS(AFireBallActor, AActor)

AFireBallActor::AFireBallActor()
{
}

void AFireBallActor::InitDefaultComponents()
{
	StaticMeshComp = AddComponent<UStaticMeshComponent>();
	SetRootComponent(StaticMeshComp);

	ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
	UStaticMesh* DefaultMesh = FObjManager::LoadObjStaticMesh("Data/BasicShape/Sphere.OBJ", Device);
	StaticMeshComp->AddWorldOffset(FVector(0.f, 0.f, 10.f));
	if (DefaultMesh)
	{
		StaticMeshComp->SetStaticMesh(DefaultMesh);
	}

	DecalComp = AddComponent<UDecalComponent>();
	DecalComp->AttachToComponent(StaticMeshComp);
	DecalComp->SetRelativeRotation(FVector(0.f, 90.f, 0.f));

	SpriteComp = AddComponent<UBillboardComponent>();
	SpriteComp->SetTexture("DecalActor");
	SpriteComp->AttachToComponent(DecalComp);
}

void AFireBallActor::TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	SetActorLocation(GetActorLocation() + FVector(0.0f, 1.0f, 0.0f) * DeltaSeconds);
}



