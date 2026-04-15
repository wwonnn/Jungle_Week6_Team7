#include "SpotLightDecalComponent.h"
#include "Render/Proxy/SpotLightDecalProxy.h"      // Step 2에서 만들 프록시
#include "Render/Resource/MeshBufferManager.h"
#include "Serialization/Archive.h"

IMPLEMENT_CLASS(USpotLightDecalComponent, UPrimitiveComponent)

FMeshBuffer* USpotLightDecalComponent::GetMeshBuffer() const
{
	// Cube 메시를 볼륨으로 사용
	EMeshShape Shape = EMeshShape::Cube;
	return &FMeshBufferManager::Get().GetMeshBuffer(Shape);
}

FPrimitiveSceneProxy* USpotLightDecalComponent::CreateSceneProxy()
{
	return new FSpotLightDecalProxy(this);
}

void USpotLightDecalComponent::Serialize(FArchive& Ar)
{
	UPrimitiveComponent::Serialize(Ar);
	Ar << InnerConeAngle;
	Ar << OuterConeAngle;
	Ar << Intensity;
	Ar << LightColor;
	Ar << AttenuationRadius;
}

void USpotLightDecalComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UPrimitiveComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "InnerConeAngle",    EPropertyType::Float, &InnerConeAngle,    0.1f,  89.0f, 0.5f });
	OutProps.push_back({ "OuterConeAngle",    EPropertyType::Float, &OuterConeAngle,    0.1f,  89.0f, 0.5f });
	OutProps.push_back({ "Intensity",         EPropertyType::Float, &Intensity,         0.0f, 100.0f, 0.1f });
	OutProps.push_back({ "LightColor",        EPropertyType::Vec3,  &LightColor });
	OutProps.push_back({ "AttenuationRadius", EPropertyType::Float, &AttenuationRadius, 1.0f, 100000.0f, 10.0f });
}