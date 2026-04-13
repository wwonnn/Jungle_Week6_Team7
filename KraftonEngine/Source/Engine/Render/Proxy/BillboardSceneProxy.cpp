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

	// ==========================================================
	// 1. 카메라와의 거리 기반 스케일(DistanceScale) 계산
	// ==========================================================
	// (주의: 엔진에 구현된 카메라 위치 함수명에 맞춰 GetCameraPosition()을 수정하세요)
	const FVector CameraPos = Bus.GetCameraPosition();
	const FVector BillboardPos = Comp->GetWorldLocation();
	const float Distance = (BillboardPos - CameraPos).Length();

	const float ThresholdDistance = 1000.0f; // 이 거리 안으로 들어오면 작아지기 시작
	float DistanceScale = 1.0f;

	if (!Bus.IsOrtho())
	{
		// 카메라가 가까워질수록 DistanceScale이 1.0에서 0.01까지 줄어듦
		if (Distance < ThresholdDistance)
		{
			DistanceScale = std::max(0.01f, Distance / ThresholdDistance);
		}
	}
	else
	{
		// 직교 투영(Orthographic) 모드일 때의 화면 크기 유지 보정
		DistanceScale = Bus.GetOrthoWidth() * 0.01f;
	}

	// ==========================================================
	// 2. 원본 로직 유지: 카메라 페이싱 회전 & 행렬 합성
	// ==========================================================
	FVector BillboardForward = Bus.GetCameraForward() * -1.0f;
	FMatrix RotMatrix;
	RotMatrix.SetAxes(BillboardForward, Bus.GetCameraRight() * -1.0f, Bus.GetCameraUp());

	// 컴포넌트의 원본 스케일에 방금 구한 거리 스케일을 곱해줍니다!
	const float IconSizeMultiplier = 100.0f;
	FVector FinalScale = Comp->GetWorldScale() * DistanceScale * IconSizeMultiplier;

	FMatrix BillboardMatrix = FMatrix::MakeScaleMatrix(FinalScale)
		* RotMatrix * FMatrix::MakeTranslationMatrix(BillboardPos);

	PerObjectConstants = FPerObjectConstants::FromWorldMatrix(BillboardMatrix);
	MarkPerObjectCBDirty();
}
