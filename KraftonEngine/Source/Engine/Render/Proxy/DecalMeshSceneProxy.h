#pragma once
#include "Render/Proxy/PrimitiveSceneProxy.h"

class UMeshDecalComponent;

class FDecalMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	FDecalMeshSceneProxy(UMeshDecalComponent* InComponent);

	virtual void UpdateMaterial() override;
	virtual void UpdateMesh() override;
	virtual void UpdateVisibility() override;
	virtual void UpdateTransform() override;

private:
	UMeshDecalComponent* GetDecalComponent() const;
};
