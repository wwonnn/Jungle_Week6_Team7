#include "DecalComponent.h"
#include "Render/Proxy/DecalSceneProxy.h"
#include "Render/Resource/MeshBufferManager.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

FMeshBuffer* UDecalComponent::GetMeshBuffer() const
{
	EMeshShape Shape = EMeshShape::Cube;
	return &FMeshBufferManager::Get().GetMeshBuffer(Shape);
}

FPrimitiveSceneProxy* UDecalComponent::CreateSceneProxy()
{
	return new FDecalSceneProxy(this);
}
