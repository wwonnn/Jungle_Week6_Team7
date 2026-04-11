#include "DecalComponent.h"
#include "Render/Proxy/DecalSceneProxy.h"
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
	return new FDecalSceneProxy(this);
}

void UDecalComponent::SetTexture(const FName& InTextureName)
{
	TextureName = InTextureName;
	CachedTexture = FResourceManager::Get().FindTexture(InTextureName);
	// FMeshSectionDraw 갱신
	MarkProxyDirty(EDirtyFlag::Material);
}

ID3D11ShaderResourceView* UDecalComponent::GetSRV() const
{
	return CachedTexture ? CachedTexture->SRV : nullptr;
}

void UDecalComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UPrimitiveComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "Texture", EPropertyType::Name, &TextureName });
	OutProps.push_back({ "Width",  EPropertyType::ByteBool, &bIsVisible });
}

void UDecalComponent::PostEditProperty(const char* PropertyName)
{
	UPrimitiveComponent::PostEditProperty(PropertyName);

	if (strcmp(PropertyName, "Texture") == 0)
	{
		SetTexture(TextureName);
	}
}
