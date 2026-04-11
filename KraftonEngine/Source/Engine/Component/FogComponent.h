#pragma once

#include "Component/MeshComponent.h"
#include "Core/PropertyTypes.h"
#include "Mesh/ObjManager.h"
class UFogComponent : public UPrimitiveComponent
{
public: 
	DECLARE_CLASS(UFogComponent, UPrimitiveComponent)
	UFogComponent();
	void GetEditableProperties(TArray<struct FPropertyDescriptor>& OutProperties);
	void PostEditProperty(const char* PropertyName);
	class FPrimitiveSceneProxy* CreateSceneProxy() override;

	float GetFogDensity() { return FogDensity; }
	float GetFogHeightFalloff() { return FogHeightFalloff; }
	float GetStartDistance(){ return FogStartDistance; }
	float GetFogCutoffDistance() { return FogCutoffDistance; }
	float GetFogMaxOpacity() { return FogMaxOpacity; }
	void SetFogDensity(float density);
	void SetFogHeightFalloff(float falloff);
	void SetStartDistance(float distance);
	void SetFogCutoffDistance(float cutoffDistance);
	void SetFogMaxOpacity(float maxOpacity);
	FVector4 GetFogColor() { return FogColor; }
	void SetFogColor(const FVector4& color) { FogColor = color; }
private:
	FVector4 FogColor;
	float FogDensity;
	float FogHeightFalloff;
	float FogStartDistance;
	float FogCutoffDistance;
	float FogMaxOpacity;

};

