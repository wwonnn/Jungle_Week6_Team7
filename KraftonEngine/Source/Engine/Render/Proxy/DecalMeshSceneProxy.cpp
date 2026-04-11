#include "DecalMeshSceneProxy.h"
#include "Component/MeshDecalComponent.h"
#include "Materials/Material.h"
#include "Texture/Texture2D.h"
#include "Render/Resource/ShaderManager.h"

FDecalMeshSceneProxy::FDecalMeshSceneProxy(UMeshDecalComponent* InComponent) : FPrimitiveSceneProxy(InComponent)
{
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
