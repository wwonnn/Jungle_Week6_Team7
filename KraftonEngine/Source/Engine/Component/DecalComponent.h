#pragma once

#include "Component/PrimitiveComponent.h"

class UDecalComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

	UDecalComponent() = default;
	~UDecalComponent() override = default;
};

