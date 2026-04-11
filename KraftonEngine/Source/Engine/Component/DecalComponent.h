#pragma once

#include "Component/PrimitiveComponent.h"

class FPrimitiveSceneProxy;

class UDecalComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

	UDecalComponent() = default;
	~UDecalComponent() override = default;

	FMeshBuffer* GetMeshBuffer() const override;
	FPrimitiveSceneProxy* CreateSceneProxy() override;
};

