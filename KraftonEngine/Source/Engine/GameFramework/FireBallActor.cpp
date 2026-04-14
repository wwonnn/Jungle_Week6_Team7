#include "FireBallActor.h"
#include "Component/StaticMeshComponent.h"
#include "Component/FireBallComponent.h"
#include "Mesh/ObjManager.h"
#include "Engine/Runtime/Engine.h"
IMPLEMENT_CLASS(AFireBallActor, AActor)

AFireBallActor::AFireBallActor()
{
}

void AFireBallActor::InitDefaultComponents()
{
	FireBallComp = AddComponent<UFireBallComponent>();
	SetRootComponent(FireBallComp);
	StaticMeshComp = AddComponent<UStaticMeshComponent>();
	StaticMeshComp->AttachToComponent(FireBallComp);

	ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
	UStaticMesh* DefaultMesh = FObjManager::LoadObjStaticMesh("Data/BasicShape/Sphere.OBJ", Device);
	StaticMeshComp->AddWorldOffset(FVector(0.f, 0.f, 10.f));
	if (DefaultMesh)
	{
		StaticMeshComp->SetStaticMesh(DefaultMesh);
	}

}

void AFireBallActor::TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	SetActorLocation(GetActorLocation() + FVector(0.0f, 1.0f, 0.0f) * DeltaSeconds);
}



    