#include "DecalComponent.h"
#include "Render/Resource/MeshBufferManager.h"
#include "Serialization/Archive.h"
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

void UDecalComponent::Serialize(FArchive& Ar)
{
	UPrimitiveComponent::Serialize(Ar);
	Ar << TextureName;
	Ar << bHasFade;
	Ar << FadeInner;
	Ar << FadeOuter;
}

void UDecalComponent::PostDuplicate()
{
	UPrimitiveComponent::PostDuplicate();
	// 텍스처 SRV 재바인딩
	SetTexture(TextureName);
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

uint64 UDecalComponent::CalculateOBBScreenPixels(const FMatrix& ViewProj, float ViewportWidth, float ViewportHeight)
{
	FVector Corners[8];
	WorldOBB.GetCorners(Corners);

	float NDCMinX = FLT_MAX, NDCMinY = FLT_MAX;
	float NDCMaxX = -FLT_MAX, NDCMaxY = -FLT_MAX;

	for (int i = 0; i < 8; ++i)
	{
		// World → Clip Space
		FVector4 Clip = ViewProj.TransformPositionWithW(Corners[i]);

		// 카메라 뒤에 있으면 스킵
		if (Clip.W <= 0.1f) continue;

		// ÷w → NDC
		float InvW = 1.f / Clip.W;
		float NDCx = Clip.X * InvW;
		float NDCy = -Clip.Y * InvW;; // Y 반전 (DX UV 방향)

		NDCMinX = std::min(NDCMinX, NDCx);
		NDCMinY = std::min(NDCMinY, NDCy);
		NDCMaxX = std::max(NDCMaxX, NDCx);
		NDCMaxY = std::max(NDCMaxY, NDCy);
	}

	// 화면 밖 클램핑
	NDCMinX = std::max(NDCMinX, -1.0f);
	NDCMinY = std::max(NDCMinY, -1.0f);
	NDCMaxX = std::min(NDCMaxX, 1.0f);
	NDCMaxY = std::min(NDCMaxY, 1.0f);

	if (NDCMaxX <= NDCMinX || NDCMaxY <= NDCMinY)
		return 0; // 화면 밖

	// NDC → Screen 픽셀
	float Width = (NDCMaxX - NDCMinX) * 0.5f * ViewportWidth;
	float Height = (NDCMaxY - NDCMinY) * 0.5f * ViewportHeight;

	return static_cast<UINT64>(Width * Height);
}
