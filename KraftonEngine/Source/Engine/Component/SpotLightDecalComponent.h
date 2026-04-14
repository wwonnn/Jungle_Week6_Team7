#pragma once

#include "Component/PrimitiveComponent.h"
#include "Core/ResourceTypes.h"
class FPrimitiveSceneProxy;
class USpotLightDecalComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(USpotLightDecalComponent, UPrimitiveComponent)
	USpotLightDecalComponent() = default;
	~USpotLightDecalComponent() override = default;
	void Serialize(FArchive& Ar) override;

protected:

	float bFade = false;
	FTextureResource* Texture = nullptr;
};
