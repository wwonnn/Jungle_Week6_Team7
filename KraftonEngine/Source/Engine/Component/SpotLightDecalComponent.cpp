#include "SpotLightDecalComponent.h"
#include "Render/Proxy/SpotLightDecalProxy.h"
#include "Render/Resource/MeshBufferManager.h"
#include "Render/Pipeline/RenderBus.h"
#include "Render/Pipeline/RenderConstants.h"
#include "Serialization/Archive.h"
#include <cmath>

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

void USpotLightDecalComponent::CollectEditorVisualizations(FRenderBus& RenderBus) const
{
	static constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
	static constexpr int32 kSegments = 24;

	const float radius = AttenuationRadius;
	const float outerRad = OuterConeAngle * kDegToRad;
	const float innerRad = InnerConeAngle * kDegToRad;
	const float outerR = tanf(outerRad) * radius;
	const float innerR = tanf(innerRad) * radius;

	const FMatrix W = GetWorldMatrix();
	const FVector Forward = FVector(W.M[0][0], W.M[0][1], W.M[0][2]).Normalized();
	const FVector Right   = FVector(W.M[1][0], W.M[1][1], W.M[1][2]).Normalized();
	const FVector Up      = FVector(W.M[2][0], W.M[2][1], W.M[2][2]).Normalized();
	const FVector Pos     = W.GetLocation();
	const FVector Tip     = Pos + Forward * radius;

	const FColor OuterColor = FColor(255, 200, 0);   // 노란색: Outer Cone
	const FColor InnerColor = FColor(0, 200, 255);    // 하늘색: Inner Cone

	// 원형 링 + 라이트 원점→링 연결선
	auto DrawCone = [&](float coneR, const FColor& Color)
	{
		FVector Prev = Tip + Right * coneR;
		for (int32 i = 1; i <= kSegments; ++i)
		{
			float angle = (float)i / (float)kSegments * 6.28318530718f;
			FVector Cur = Tip + Right * (cosf(angle) * coneR) + Up * (sinf(angle) * coneR);

			// 원형 링
			FDebugLineEntry Ring;
			Ring.Start = Prev;
			Ring.End   = Cur;
			Ring.Color = Color;
			RenderBus.AddDebugLineEntry(std::move(Ring));

			Prev = Cur;
		}

		// 4개 직선: 원점 → 링의 상하좌우
		FVector Dirs[4] = {
			Tip + Right * coneR,
			Tip - Right * coneR,
			Tip + Up * coneR,
			Tip - Up * coneR
		};
		for (int32 i = 0; i < 4; ++i)
		{
			FDebugLineEntry Line;
			Line.Start = Pos;
			Line.End   = Dirs[i];
			Line.Color = Color;
			RenderBus.AddDebugLineEntry(std::move(Line));
		}
	};

	DrawCone(outerR, OuterColor);
	DrawCone(innerR, InnerColor);

	// Forward 방향 중심선
	FDebugLineEntry Center;
	Center.Start = Pos;
	Center.End   = Tip;
	Center.Color = FColor(255, 255, 255);
	RenderBus.AddDebugLineEntry(std::move(Center));
}