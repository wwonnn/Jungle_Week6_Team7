#pragma once
#include "GameFramework/AActor.h"

class UDecalComponent;
class UStaticMeshComponent;
class UBillboardComponent;
class AFireBallActor : public AActor
{
public:
	DECLARE_CLASS(ADecalActor, AActor)
	AFireBallActor();
	void InitDefaultComponents();
	virtual void TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction);

private:
	UDecalComponent* DecalComp = nullptr;
	UStaticMeshComponent* StaticMeshComp = nullptr;
	UBillboardComponent* SpriteComp = nullptr;
};