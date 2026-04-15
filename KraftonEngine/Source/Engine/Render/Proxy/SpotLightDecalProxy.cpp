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
}

USpotLightDecalComponent* FSpotLightDecalProxy::GetLightComponent() const
{
	return static_cast<USpotLightDecalComponent*>(Owner);
}
