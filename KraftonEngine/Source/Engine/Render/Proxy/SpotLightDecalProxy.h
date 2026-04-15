#pragma once
#include "Render/Proxy/PrimitiveSceneProxy.h"

class USpotLightDecalComponent;

class FSpotLightDecalProxy : public FPrimitiveSceneProxy
{
public:
	FSpotLightDecalProxy(USpotLightDecalComponent* InComponent);

	void UpdateMaterial() override;
	void UpdateMesh() override;
	void UpdatePerViewport(const FRenderBus& Bus) override;

private:
	USpotLightDecalComponent* GetLightComponent() const;
};