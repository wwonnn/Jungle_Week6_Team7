#include "DecalSceneProxy.h"
#include "Component/DecalComponent.h"
#include "Render/Resource/ShaderManager.h"
#include "Render/Pipeline/RenderBus.h"
#include "Render/Pipeline/RenderConstants.h"
#include "Render/Resource/ConstantBufferPool.h"

FDecalSceneProxy::FDecalSceneProxy(UDecalComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	bPerViewportUpdate = true;
	bSupportsOutline = false;

	FMeshSectionDraw DecalSection;

	DecalSection.DiffuseSRV = GetDecalComponent()->GetSRV();
	DecalSection.DiffuseColor = { 1.f, 1.f, 1.f, 1.f }; // 텍스처 그대로
	DecalSection.FirstIndex = 0;               
	DecalSection.IndexCount = 36;               
	DecalSection.bIsUVScroll = false;

	SectionDraws.push_back(DecalSection);
}

UDecalComponent* FDecalSceneProxy::GetDecalComponent() const
{
	return static_cast<UDecalComponent*>(Owner);
}

void FDecalSceneProxy::UpdateMaterial()
{
	SectionDraws[0].DiffuseSRV = GetDecalComponent()->GetSRV();
}

void FDecalSceneProxy::UpdateMesh()
{
	MeshBuffer = Owner->GetMeshBuffer();
	Shader = FShaderManager::Get().GetShader(EShaderType::Decal);
	Pass = ERenderPass::Decal;
	UpdateSortKey();
}

void FDecalSceneProxy::UpdatePerViewport(const FRenderBus& Bus)
{
	// ExtraCB — FDecalConstants
	auto& G = ExtraCB.Bind<FDecalConstants>(
		FConstantBufferPool::Get().GetBuffer(ECBSlot::Decal, sizeof(FDecalConstants)),
		ECBSlot::Decal);

	G.WorldToDecal = PerObjectConstants.Model.GetInverse();
	GetDecalComponent()->SetFadeConstants(G);
}

