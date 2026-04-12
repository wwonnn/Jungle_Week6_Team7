#include "DecalComponent.h"
#include "Render/Resource/MeshBufferManager.h"
#include "Serialization/Archive.h"
#include "Texture/Texture2D.h"
#include "Engine/Runtime/Engine.h"
#include "Resource/ResourceManager.h"
#include "Core/ResourceTypes.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

FMeshBuffer* UDecalComponent::GetMeshBuffer() const
{
	EMeshShape Shape = EMeshShape::Cube;
	return &FMeshBufferManager::Get().GetMeshBuffer(Shape);
}

FPrimitiveSceneProxy* UDecalComponent::CreateSceneProxy()
{
	SetTexture(TextureName);
	return new FDecalSceneProxy(this);;
}

void UDecalComponent::SetTexture(const FName& InTextureName)
{
	TextureName = InTextureName;
	CachedTexture = FResourceManager::Get().FindTexture(InTextureName);
	// FMeshSectionDraw 갱신
	MarkProxyDirty(EDirtyFlag::Material);
}

void UDecalComponent::SetFadeConstants(FDecalConstants& OutDecalConstants) const
{
	OutDecalConstants.FadeInner = bHasFade ? FadeInner : 1.0f;
	OutDecalConstants.FadeOuter = bHasFade ? FadeOuter : 1.0f;
}

ID3D11ShaderResourceView* UDecalComponent::GetSRV() const
{
	return CachedTexture ? CachedTexture->SRV : nullptr;
}

void UDecalComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UPrimitiveComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "Texture", EPropertyType::Name, &TextureName });
	OutProps.push_back({ "Fade",  EPropertyType::ByteBool, &bHasFade });
	OutProps.push_back({ "Fade Inner",  EPropertyType::Float, &FadeInner, 0.0f, FadeOuter, 0.01f });
	OutProps.push_back({ "Fade Outer",  EPropertyType::Float, &FadeOuter, FadeInner, 1.0f, 0.01f });
}

void UDecalComponent::PostEditProperty(const char* PropertyName)
{
	UPrimitiveComponent::PostEditProperty(PropertyName);

	if (strcmp(PropertyName, "Texture") == 0)
	{
		SetTexture(TextureName);
	}
}
