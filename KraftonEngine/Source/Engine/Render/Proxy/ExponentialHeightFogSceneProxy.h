
#include "Component/ExponentialHeightFogComponent.h"
#include "Render/Resource/ShaderManager.h"
#include "Render/Resource/ConstantBufferPool.h"
#include "PrimitiveSceneProxy.h"

class FRenderBus;
class FPrimitiveSceneProxy;
class FExponentialHeightFogSceneProxy : public FPrimitiveSceneProxy
{
public:
	FExponentialHeightFogSceneProxy(UExponentialHeightFogComponent* InComponent);

	void UpdateMesh() override;


	void UpdatePerViewport(const FRenderBus& Bus) override;

};

