#include "HeightFogComponent.h"
#include "Serialization/Archive.h"

IMPLEMENT_CLASS(UHeightFogComponent, USceneComponent)

void UHeightFogComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	USceneComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "FogDensity",           EPropertyType::Float, &FogDensity,           0.0f,  5.0f,   0.001f });
	OutProps.push_back({ "FogHeightFalloff",     EPropertyType::Float, &FogHeightFalloff,     0.001f, 5.0f,  0.01f });
	OutProps.push_back({ "StartDistance",         EPropertyType::Float, &StartDistance,         0.0f, 100000.0f, 10.0f });
	OutProps.push_back({ "FogCutoffDistance",     EPropertyType::Float, &FogCutoffDistance,     0.0f, 100000.0f, 10.0f });
	OutProps.push_back({ "FogMaxOpacity",         EPropertyType::Float, &FogMaxOpacity,         0.0f,  1.0f,   0.01f });
	OutProps.push_back({ "FogColor R",            EPropertyType::Float, &FogInscatteringColor.X, 0.0f, 1.0f, 0.01f });
	OutProps.push_back({ "FogColor G",            EPropertyType::Float, &FogInscatteringColor.Y, 0.0f, 1.0f, 0.01f });
	OutProps.push_back({ "FogColor B",            EPropertyType::Float, &FogInscatteringColor.Z, 0.0f, 1.0f, 0.01f });
}

void UHeightFogComponent::Serialize(FArchive& Ar)
{
	USceneComponent::Serialize(Ar);
	Ar << FogDensity;
	Ar << FogHeightFalloff;
	Ar << StartDistance;
	Ar << FogCutoffDistance;
	Ar << FogMaxOpacity;
	Ar << FogInscatteringColor;
}
