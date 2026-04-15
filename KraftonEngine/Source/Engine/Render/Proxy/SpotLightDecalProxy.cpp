#include "SpotLightDecalProxy.h"
#include "Component/SpotLightDecalComponent.h"
#include "Render/Resource/ShaderManager.h"
#include "Render/Pipeline/RenderBus.h"
#include "Render/Pipeline/RenderConstants.h"
#include "Render/Resource/ConstantBufferPool.h"
#include <cmath>

static constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

FSpotLightDecalProxy::FSpotLightDecalProxy(USpotLightDecalComponent* InComponent) : FPrimitiveSceneProxy(InComponent)
{
	bPerViewportUpdate = true;   // 매 프레임 카메라에 따라 CB 갱신
	bSupportsOutline = false;  // 스텐실 아웃라인 불필요

	// Cube mesh: 36 인덱스 (12삼각형 × 3)
	FMeshSectionDraw Section;
	Section.DiffuseSRV = nullptr;  // 텍스처 없음 (DepthSRV만 사용)
	Section.DiffuseColor = { 1.f, 1.f, 1.f, 1.f };
	Section.FirstIndex = 0;
	Section.IndexCount = 36;
	Section.bIsUVScroll = false;
	SectionDraws.push_back(Section);
}


void FSpotLightDecalProxy::UpdateMaterial()
{
}

void FSpotLightDecalProxy::UpdateMesh()
{
	MeshBuffer = Owner->GetMeshBuffer();
	Shader = FShaderManager::Get().GetShader(EShaderType ::SpotLightDecal);
	Pass = ERenderPass::SpotLightDecal;
	UpdateSortKey();
}

void FSpotLightDecalProxy::UpdatePerViewport(const FRenderBus& Bus)
{
	USpotLightDecalComponent* Comp = GetLightComponent();

	float outerAngleRad = Comp->GetOuterConeAngle() * kDegToRad;
	float innerAngleRad = Comp->GetInnerConeAngle() * kDegToRad;
	float radius = Comp->GetAttenuationRadius();
	float coneRadius = tanf(outerAngleRad) * radius;

	// ── 컴포넌트 월드 행렬에서 축 추출 ──
	FMatrix ComponentWorld = Comp->GetWorldMatrix();
	FVector Forward = FVector(ComponentWorld.M[0][0], ComponentWorld.M[0][1], ComponentWorld.M[0][2]).Normalized();
	FVector Right = FVector(ComponentWorld.M[1][0], ComponentWorld.M[1][1], ComponentWorld.M[1][2]).Normalized();
	FVector Up = FVector(ComponentWorld.M[2][0], ComponentWorld.M[2][1], ComponentWorld.M[2][2]).Normalized();
	FVector Pos = ComponentWorld.GetLocation();

	// ── Cube [-0.5, 0.5] → 콘 볼륨 변환 행렬 ──
	FMatrix WorldMatrix;
	// Row 0: X축(Forward) = 라이트 방향 × 반경
	WorldMatrix.M[0][0] = Forward.X * radius;
	WorldMatrix.M[0][1] = Forward.Y * radius;
	WorldMatrix.M[0][2] = Forward.Z * radius;
	WorldMatrix.M[0][3] = 0.0f;
	// Row 1: Y축(Right) = 콘의 좌우 폭
	WorldMatrix.M[1][0] = Right.X * coneRadius * 2.0f;
	WorldMatrix.M[1][1] = Right.Y * coneRadius * 2.0f;
	WorldMatrix.M[1][2] = Right.Z * coneRadius * 2.0f;
	WorldMatrix.M[1][3] = 0.0f;
	// Row 2: Z축(Up) = 콘의 상하 폭
	WorldMatrix.M[2][0] = Up.X * coneRadius * 2.0f;
	WorldMatrix.M[2][1] = Up.Y * coneRadius * 2.0f;
	WorldMatrix.M[2][2] = Up.Z * coneRadius * 2.0f;
	WorldMatrix.M[2][3] = 0.0f;
	// Row 3: Position = 라이트 위치에서 Forward 방향으로 반경/2 이동 (Cube 중심 보정)
	WorldMatrix.M[3][0] = Pos.X + Forward.X * radius * 0.5f;
	WorldMatrix.M[3][1] = Pos.Y + Forward.Y * radius * 0.5f;
	WorldMatrix.M[3][2] = Pos.Z + Forward.Z * radius * 0.5f;
	WorldMatrix.M[3][3] = 1.0f;

	// PerObject CB 업데이트 (VS에서 MVP 변환에 사용)
	PerObjectConstants.Model = WorldMatrix;
	MarkPerObjectCBDirty();

	// ── Extra CB: SpotLightDecal 전용 상수 (b7 슬롯) ──
	auto& CB = ExtraCB.Bind<FSpotLightDecalConstants>(
		FConstantBufferPool::Get().GetBuffer(ECBSlot::SpotLightDecal, sizeof(FSpotLightDecalConstants)),
		ECBSlot::SpotLightDecal);

	// Light의 순수 Transform (스케일 및 오프셋 미적용) 구성
	FMatrix LightTransform;
	LightTransform.M[0][0] = Forward.X; LightTransform.M[0][1] = Forward.Y; LightTransform.M[0][2] = Forward.Z; LightTransform.M[0][3] = 0.0f;
	LightTransform.M[1][0] = Right.X;   LightTransform.M[1][1] = Right.Y;   LightTransform.M[1][2] = Right.Z;   LightTransform.M[1][3] = 0.0f;
	LightTransform.M[2][0] = Up.X;      LightTransform.M[2][1] = Up.Y;      LightTransform.M[2][2] = Up.Z;      LightTransform.M[2][3] = 0.0f;
	LightTransform.M[3][0] = Pos.X;     LightTransform.M[3][1] = Pos.Y;     LightTransform.M[3][2] = Pos.Z;     LightTransform.M[3][3] = 1.0f;

	CB.WorldToLight = LightTransform.GetInverse();  // World -> Unscaled Light Local 역행렬
	CB.LightColor = Comp->GetLightColor();
	CB.Intensity = Comp->GetIntensity();
	CB.InnerConeAngleCos = cosf(innerAngleRad);       // CPU에서 미리 cos 계산
	CB.OuterConeAngleCos = cosf(outerAngleRad);
	CB.AttenuationRadius = radius;
}

USpotLightDecalComponent* FSpotLightDecalProxy::GetLightComponent() const
{
	return static_cast<USpotLightDecalComponent*>(Owner);
}
