#pragma once
#include "Component/PrimitiveComponent.h"
#include "Core/PropertyTypes.h"
#include "Materials/Material.h"
#include "Math/Vector.h"
#include "Render/Resource/Buffer.h"
class UMeshDecalComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UMeshDecalComponent, UPrimitiveComponent)

	UMeshDecalComponent();

	virtual FMeshBuffer* GetMeshBuffer() const override { return const_cast<FMeshBuffer*>(&MeshBuffer); }
	virtual const FMeshData* GetMeshData() const override { return nullptr; }
	bool LineTraceComponent(const FRay& Ray, FHitResult& OutHitResult) override;
	UMaterial* GetMaterial() const { return Material; }

	void UpdateDecalMeshData();
	void ClipByDecalLocalOBB(TArray<FVertexPNCT>& OutClippedVertices, const FVertexPNCT& P0, const FVertexPNCT& P1, const FVertexPNCT& P2);
	void ClipByAxis(float HExtent, uint32 AxisType, const FVertexPNCT& A, const FVertexPNCT& B, bool Larger, TArray<FVertexPNCT>& OutClippedVertices);
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
	void PostEditProperty(const char* PropertyName) override;
	void PostDuplicate() override;
	void CollectEditorVisualizations(FRenderBus& RenderBus) const override;
	virtual void BeginPlay() override;

private:
	void RefreshMaterial();

	FMaterialSlot MaterialSlot;
	UMaterial* Material = nullptr;
	TMeshData<FVertexPNCT> DecalMeshData;
	FMeshBuffer MeshBuffer;

private:
	TArray<UPrimitiveComponent*> GetCandidates() const;

protected:
	void OnTransformDirty() override;
};
