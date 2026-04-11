
#include "Component/FogComponent.h"
#include "Render/Resource/ShaderManager.h"
#include "Render/Resource/ConstantBufferPool.h"
#include "PrimitiveSceneProxy.h"

class FRenderBus;
class FPrimitiveSceneProxy;
class FFogSceneProxy : public FPrimitiveSceneProxy
{
public:
	FFogSceneProxy(UFogComponent* InComponent);

	void UpdateMesh() override;


	void UpdatePerViewport(const FRenderBus& Bus) override;

};

