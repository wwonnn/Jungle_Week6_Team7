#include "FogSceneProxy.h"
#include "Render/Pipeline/RenderBus.h"
FFogSceneProxy::FFogSceneProxy(UFogComponent* InComponent) : FPrimitiveSceneProxy(InComponent)
{
	Pass = ERenderPass::PostProcess;
	bNeverCull = true; 
	bPerViewportUpdate = true;
}

void FFogSceneProxy::UpdateMesh()
{
	// 포그는 버텍스 버퍼가 없는 Fullscreen Triangle로 그림
	MeshBuffer = nullptr;
	Shader = FShaderManager::Get().GetShader(EShaderType::Fog);
	UpdateSortKey();
}

void FFogSceneProxy::UpdatePerViewport(const FRenderBus& Bus)
{
	UFogComponent* FogComp = static_cast<UFogComponent*>(Owner);
	if (!Bus.GetShowFlags().bFog || !FogComp->IsVisible())
	{
		bVisible = false;
		return;
	}
	bVisible = true;

	// Fog 상수 버퍼 세팅
	auto& Fog = ExtraCB.Bind<FFogConstants>(
		FConstantBufferPool::Get().GetBuffer(ECBSlot::Fog, sizeof(FFogConstants)),
		ECBSlot::Fog);

	Fog.FogColor = FogComp->GetFogColor();
	Fog.FogDensity = FogComp->GetFogDensity();
	Fog.FogHeightFalloff = FogComp->GetFogHeightFalloff();
	Fog.StartDistance = FogComp->GetStartDistance();
	Fog.FogCutoffDistance = FogComp->GetFogCutoffDistance();
	Fog.FogMaxOpacity = FogComp->GetFogMaxOpacity();

	Fog.InvViewProj = (Bus.GetView() * Bus.GetProj()).GetInverse();
	Fog.CameraPos = FVector4(Bus.GetCameraPosition(), 1.0f);
}
