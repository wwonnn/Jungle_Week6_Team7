#include "Render/Proxy/BillboardSceneProxy.h"
#include "Component/BillboardComponent.h"
#include "Render/Resource/ShaderManager.h"
#include "Render/Pipeline/RenderBus.h"
#include "Render/Pipeline/RenderConstants.h"

// ============================================================
// FBillboardSceneProxy
// ============================================================
FBillboardSceneProxy::FBillboardSceneProxy(UBillboardComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	bPerViewportUpdate = true;
	bShowAABB = false;
	// 텍스처가 있으면 Batcher 경로, 없으면 기본 Primitive 경로
	bBatcherRendered = (InComponent && InComponent->GetTexture() != nullptr);
}

UBillboardComponent* FBillboardSceneProxy::GetBillboardComponent() const
{
	return static_cast<UBillboardComponent*>(Owner);
}

// ============================================================
// UpdateMesh — SelectionMask를 위한 셰이더 및 섹션 설정
// ============================================================
void FBillboardSceneProxy::UpdateMesh()
{
	MeshBuffer = Owner->GetMeshBuffer();

	UBillboardComponent* Comp = GetBillboardComponent();
	const bool bHasTexture = (Comp && Comp->GetTexture() != nullptr);
	bBatcherRendered = bHasTexture;

	if (bHasTexture)
	{
		// 1. SelectionMask(아웃라인) 시 알파 컷오프를 지원하는 Billboard 셰이더 사용
		Shader = FShaderManager::Get().GetShader(EShaderType::Billboard);
		
		// 2. 렌더 패스는 Billboard로 설정 (기즈모 위 렌더링)
		Pass = ERenderPass::Billboard; 

		// 3. SelectionMask 패스에서 텍스처 바인딩을 위한 정보 설정
		SectionDraws.clear();
		FMeshSectionDraw DecalSection;
		DecalSection.DiffuseSRV = Comp->GetTexture()->SRV;
		DecalSection.DiffuseColor = { 1.f, 1.f, 1.f, 1.f };
		DecalSection.FirstIndex = 0;
		DecalSection.IndexCount = 6; // Quad(2 Triangles)
		DecalSection.bIsUVScroll = false;
		SectionDraws.push_back(DecalSection);
	}
	else
	{
		Shader = FShaderManager::Get().GetShader(EShaderType::Primitive);
		Pass = ERenderPass::Opaque;
	}
	UpdateSortKey();
}

void FBillboardSceneProxy::CollectEntries(FRenderBus& Bus)
{
	UBillboardComponent* Comp = GetBillboardComponent();
	if (!Comp) return;

	const FTextureResource* Texture = Comp->GetTexture();
	if (!Texture || !Texture->IsLoaded()) return;

	FBillboardEntry Entry = {};
	Entry.PerObject = PerObjectConstants;
	Entry.Billboard.Texture = Texture;
	Entry.Billboard.Width   = Comp->GetWidth();
	Entry.Billboard.Height  = Comp->GetHeight();
	Bus.AddBillboardEntry(std::move(Entry));
}

void FBillboardSceneProxy::UpdatePerViewport(const FRenderBus& Bus)
{
	UBillboardComponent* Comp = GetBillboardComponent();
	bVisible = Comp->IsVisible();
	if (!bVisible) return;

	// 카메라 방향으로 향하는 월드 행렬 계산
	FVector BillboardForward = Bus.GetCameraForward() * -1.0f;
	FMatrix RotMatrix;
	RotMatrix.SetAxes(BillboardForward, Bus.GetCameraRight() * -1.0f, Bus.GetCameraUp());
	FMatrix BillboardMatrix = FMatrix::MakeScaleMatrix(Comp->GetWorldScale())
		* RotMatrix * FMatrix::MakeTranslationMatrix(Comp->GetWorldLocation());

	PerObjectConstants = FPerObjectConstants::FromWorldMatrix(BillboardMatrix);
	MarkPerObjectCBDirty();
}
