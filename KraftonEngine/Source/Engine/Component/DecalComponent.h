#pragma once

#include "Component/PrimitiveComponent.h"
#include "Core/ResourceTypes.h"
#include "Object/FName.h"

class FPrimitiveSceneProxy;

class UDecalComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

	UDecalComponent() = default;
	~UDecalComponent() override = default;

	FMeshBuffer* GetMeshBuffer() const override;
	FPrimitiveSceneProxy* CreateSceneProxy() override;

	void SetTexture(const FName& InTextureName);
	ID3D11ShaderResourceView* GetSRV() const;

	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
	void PostEditProperty(const char* PropertyName) override;

private:
	FName TextureName = "Pawn";
	FTextureResource* CachedTexture = nullptr;
};

