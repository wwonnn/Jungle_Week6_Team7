#include "FogComponent.h"
#include "Render/Proxy/FogSceneProxy.h"
#include "Math/Vector.h"
IMPLEMENT_CLASS(UFogComponent, UPrimitiveComponent)
UFogComponent::UFogComponent()
{
	FogColor = FVector4(0.5f, 0.6f, 0.7f, 1.0f);
	FogDensity = 0.01f;
	FogHeightFalloff = 0.1f;
	FogStartDistance = 0.0f;
	FogCutoffDistance = 10000.0f;
	FogMaxOpacity = 1.0f;

}

void UFogComponent::GetEditableProperties(TArray<struct FPropertyDescriptor>& OutProperties)
{
	UPrimitiveComponent::GetEditableProperties(OutProperties);

	OutProperties.push_back(FPropertyDescriptor{ "Fog Color", EPropertyType::Vec4, &FogColor.X });
	OutProperties.push_back(FPropertyDescriptor{ "Fog Density", EPropertyType::Float, &FogDensity, 0.0f, 1.0f, 0.001f });
	OutProperties.push_back(FPropertyDescriptor{ "Height Falloff", EPropertyType::Float, &FogHeightFalloff, 0.0f, 10.0f, 0.01f });
	OutProperties.push_back(FPropertyDescriptor{ "Start Distance", EPropertyType::Float, &FogStartDistance, 0.0f, 50000.0f, 10.0f });
	OutProperties.push_back(FPropertyDescriptor{ "Cutoff Distance", EPropertyType::Float, &FogCutoffDistance, 0.0f, 100000.0f, 100.0f });
	OutProperties.push_back(FPropertyDescriptor{ "Max Opacity", EPropertyType::Float, &FogMaxOpacity, 0.0f, 1.0f, 0.01f });

}

void UFogComponent::PostEditProperty(const char* PropertyName)
{
	UPrimitiveComponent::PostEditProperty(PropertyName);

	MarkTransformDirty();
}

void UFogComponent::SetFogDensity(float density)
{
	FogDensity = density;
}

void UFogComponent::SetFogHeightFalloff(float falloff)
{
	FogHeightFalloff = falloff;
}

void UFogComponent::SetStartDistance(float distance)
{
	FogStartDistance = distance;
}

void UFogComponent::SetFogCutoffDistance(float cutoffDistance)
{
	FogCutoffDistance = cutoffDistance;
}

void UFogComponent::SetFogMaxOpacity(float maxOpacity)
{
	FogMaxOpacity = maxOpacity;
}

FPrimitiveSceneProxy* UFogComponent::CreateSceneProxy()
{
	return new FFogSceneProxy(this);
}
