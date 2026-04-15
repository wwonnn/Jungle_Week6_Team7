#pragma once

#include "Component/PrimitiveComponent.h"
#include "Object/FName.h"
#include "Math/Vector.h"

class FPrimitiveSceneProxy;
  
// UE5 SpotLight + Projection Decal 방식의 Fake SpotLight
// Cube 볼륨을 X축 Forward로 투영, Additive 블렌딩으로 빛 누적
class USpotLightDecalComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(USpotLightDecalComponent, UPrimitiveComponent)

	USpotLightDecalComponent() = default;
	~USpotLightDecalComponent() override = default;

	FMeshBuffer* GetMeshBuffer() const override;
	FPrimitiveSceneProxy* CreateSceneProxy() override;

	void Serialize(FArchive& Ar) override;
	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;

	// Getters
	float   GetInnerConeAngle()    const { return InnerConeAngle; }
	float   GetOuterConeAngle()    const { return OuterConeAngle; }
	float   GetIntensity()         const { return Intensity; }
	FVector GetLightColor()        const { return LightColor; }
	float   GetAttenuationRadius() const { return AttenuationRadius; }

protected:
	float   InnerConeAngle    = 22.0f;   // degrees
	float   OuterConeAngle    = 44.0f;   // degrees
	float   Intensity         = 5.0f;
	FVector LightColor        = FVector(1.0f, 1.0f, 0.9f);
	float   AttenuationRadius = 1000.0f;
};
