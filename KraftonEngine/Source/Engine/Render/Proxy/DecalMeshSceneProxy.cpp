#include "DecalMeshSceneProxy.h"
#include "Component/MeshDecalComponent.h"
#include "Materials/Material.h"
#include "Texture/Texture2D.h"
#include "Render/Resource/ShaderManager.h"
#include "Render/Resource/ConstantBufferPool.h"

FDecalMeshSceneProxy::FDecalMeshSceneProxy(UMeshDecalComponent* InComponent) : FPrimitiveSceneProxy(InComponent)
{
	auto& CB = ExtraCB.Bind<FMeshDecalConstants>(
		FConstantBufferPool::Get().GetBuffer(ECBSlot::MeshDecal, sizeof(FMeshDecalConstants)),
		ECBSlot::MeshDecal);
	CB.Opacity = Cast<UMeshDecalComponent>(Owner)->GetOpacity();
}



UMeshDecalComponent* FDecalMeshSceneProxy::GetDecalComponent() const
{
	return static_cast<UMeshDecalComponent*>(Owner);
}

void FDecalMeshSceneProxy::UpdateMaterial()
{
	SectionDraws.clear();

	if (!MeshBuffer || !MeshBuffer->IsValid())
	{
		UpdateSortKey();
		return;
	}

	FMeshSectionDraw Draw;
	Draw.FirstIndex = 0;
	Draw.IndexCount = MeshBuffer->GetIndexBuffer().GetIndexCount();

	if (UMeshDecalComponent* DecalComponent = GetDecalComponent())
	{
		if (UMaterial* Material = DecalComponent->GetMaterial())
		{
			Draw.DiffuseColor = Material->DiffuseColor;
			if (Material->DiffuseTexture)
			{
				Draw.DiffuseSRV = Material->DiffuseTexture->GetSRV();
			}
		}
	}

	if (Draw.IndexCount > 0)
	{
		SectionDraws.push_back(Draw);
	}

	UpdateSortKey();
}

void FDecalMeshSceneProxy::UpdateMesh()
{
	MeshBuffer = Owner->GetMeshBuffer();
	Shader = FShaderManager::Get().GetShader(EShaderType::MeshDecal);
	Pass = ERenderPass::MeshDecal;
	UpdateMaterial();
}

void FDecalMeshSceneProxy::UpdateVisibility()
{
	FPrimitiveSceneProxy::UpdateVisibility();
}

void FDecalMeshSceneProxy::UpdateTransform()
{
	UpdateMesh();
	FPrimitiveSceneProxy::UpdateTransform();
}

void FDecalMeshSceneProxy::UpdateOpacity()
{
	ExtraCB.As<FMeshDecalConstants>().Opacity =
		Cast<UMeshDecalComponent>(Owner)->GetOpacity();
}
