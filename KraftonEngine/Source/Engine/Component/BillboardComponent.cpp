#include "BillboardComponent.h"
#include "GameFramework/World.h"
#include "Component/CameraComponent.h"
#include "Render/Proxy/BillboardSceneProxy.h"
#include "Resource/ResourceManager.h"
#include "Serialization/Archive.h"
#include "Object/ObjectFactory.h"

#include <cstring>
#include <algorithm>

IMPLEMENT_CLASS(UBillboardComponent, UPrimitiveComponent)

FPrimitiveSceneProxy* UBillboardComponent::CreateSceneProxy()
{
	return new FBillboardSceneProxy(this);
}

void UBillboardComponent::Serialize(FArchive& Ar)
{
	UPrimitiveComponent::Serialize(Ar);
	Ar << bIsBillboard;
	Ar << TextureName;
	Ar << Width;
	Ar << Height;
}

void UBillboardComponent::PostDuplicate()
{
	UPrimitiveComponent::PostDuplicate();
	// 텍스처 SRV 재바인딩
	SetTexture(TextureName);
}

void UBillboardComponent::SetTexture(const FName& InTextureName)
{
	TextureName = InTextureName;
	CachedTexture = FResourceManager::Get().FindTexture(InTextureName);
	// 텍스처 유무가 batcher/Primitive 경로 분기를 좌우하므로 Mesh 단계까지 재갱신 필요.
	MarkProxyDirty(EDirtyFlag::Mesh);
}

void UBillboardComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UPrimitiveComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "Texture", EPropertyType::Name, &TextureName });
	OutProps.push_back({ "Width",  EPropertyType::Float, &Width,  0.1f, 100.0f, 0.1f });
	OutProps.push_back({ "Height", EPropertyType::Float, &Height, 0.1f, 100.0f, 0.1f });
}

void UBillboardComponent::PostEditProperty(const char* PropertyName)
{
	UPrimitiveComponent::PostEditProperty(PropertyName);

	if (strcmp(PropertyName, "Texture") == 0)
	{
		SetTexture(TextureName);
	}
	else if (strcmp(PropertyName, "Width") == 0 || strcmp(PropertyName, "Height") == 0)
	{
		// Width/Height는 빌보드 쿼드 크기를 결정하므로 트랜스폼/월드 바운드 모두 갱신해야 한다.
		MarkProxyDirty(EDirtyFlag::Transform);
		MarkWorldBoundsDirty();
	}
}

bool UBillboardComponent::LineTraceComponent(const FRay& Ray, FHitResult& OutHitResult)
{
	if (!GetOwner() || !GetOwner()->GetWorld()) return false;
	const UCameraComponent* ActiveCamera = GetOwner()->GetWorld()->GetActiveCamera();
	if (!ActiveCamera) return false;

	// 1. 피킹 시점의 카메라 방향 기반 평면 구성
	FVector Normal = ActiveCamera->GetForwardVector() * -1.0f;
	FVector Pos = GetWorldLocation();

	float Denom = Normal.Dot(Ray.Direction);
	if (std::abs(Denom) < 1e-6f) return false;

	float t = (Pos - Ray.Origin).Dot(Normal) / Denom;
	if (t < 0) return false;

	FVector HitPoint = Ray.Origin + Ray.Direction * t;
	FVector LocalHit = HitPoint - Pos;

	// 2. 현재 BillboardSceneProxy와 완벽하게 일치하는 스케일 로직
	float Distance = (Pos - ActiveCamera->GetWorldLocation()).Length();
	
	const float BaseWorldScale = 1.0f;
	const float ScreenScaleFactor = 0.05f;
	const float MasterIconScale = 5.0f;

	float DistanceScale = BaseWorldScale;

	if (!ActiveCamera->IsOrthogonal())
	{
		float LimitScale = Distance * ScreenScaleFactor;
		float BaseRatio = std::min(BaseWorldScale, LimitScale);
		BaseRatio = std::max(0.0001f, BaseRatio);
		DistanceScale = BaseRatio * MasterIconScale;
	}
	else
	{
		DistanceScale = ActiveCamera->GetOrthoWidth() * 0.001f * BaseWorldScale * MasterIconScale;
	}

	// 3. 실제 화면에 그려지는 크기 (BillboardBatcher의 0.25 계수 반영)
	// Batcher: HalfW = Width * Scale.Y * 0.25f
	// 전체 Width = 2 * HalfW = Width * Scale.Y * 0.5f
	FVector WorldScale = GetWorldScale();
	float VisualWidth = Width * WorldScale.Y * DistanceScale * 0.5f;
	float VisualHeight = Height * WorldScale.Z * DistanceScale * 0.5f;

	// 4. Batcher가 정점을 뻗는 실제 축 (카메라의 Right, Up)
	FVector Right = ActiveCamera->GetRightVector();
	FVector Up = ActiveCamera->GetUpVector();

	float x = LocalHit.Dot(Right);
	float y = LocalHit.Dot(Up);

	// 5. 판정 범위 체크 (약간의 마진 추가 1.1배)
	const float PickingMargin = 1.1f;
	if (std::abs(x) > (VisualWidth * 0.5f) * PickingMargin || std::abs(y) > (VisualHeight * 0.5f) * PickingMargin)
		return false;

	// 6. 원형 판정
	float normX = x / (VisualWidth * 0.5f);
	float normY = y / (VisualHeight * 0.5f);
	if (normX * normX + normY * normY > 1.0f)
		return false;

	// 7. 히트 결과 설정
	OutHitResult.bHit = true;
	OutHitResult.Distance = t;
	OutHitResult.WorldHitLocation = HitPoint;
	OutHitResult.WorldNormal = Normal;
	OutHitResult.HitComponent = this;
	return true;
}

void UBillboardComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
	
{
	if (!GetOwner() || !GetOwner()->GetWorld()) return;

	const UCameraComponent* ActiveCamera = GetOwner()->GetWorld()->GetActiveCamera();
	if (!ActiveCamera) return;

	FVector WorldLocation = GetWorldLocation();
	FVector CameraForward = ActiveCamera->GetForwardVector().Normalized();
	FVector Forward = CameraForward * -1;
	FVector WorldUp = FVector(0.0f, 0.0f, 1.0f);

	if (std::abs(Forward.Dot(WorldUp)) > 0.99f)
	{
		WorldUp = FVector(0.0f, 1.0f, 0.0f); // 임시 Up축 변경
	}

	FVector Right = WorldUp.Cross(Forward).Normalized();
	FVector Up = Forward.Cross(Right).Normalized();

	FMatrix RotMatrix;
	RotMatrix.SetAxes(Forward, Right, Up);

	CachedWorldMatrix = FMatrix::MakeScaleMatrix(GetWorldScale()) * RotMatrix * FMatrix::MakeTranslationMatrix(WorldLocation);

	UpdateWorldAABB();
}

FMatrix UBillboardComponent::ComputeBillboardMatrix(const FVector& CameraForward) const
{
	// TickComponent와 동일한 로직
	FVector Forward = (CameraForward * -1.0f).Normalized();
	FVector WorldUp = FVector(0.0f, 0.0f, 1.0f);

	if (std::abs(Forward.Dot(WorldUp)) > 0.99f)
	{
		WorldUp = FVector(0.0f, 1.0f, 0.0f);
	}

	FVector Right = WorldUp.Cross(Forward).Normalized();
	FVector Up = Forward.Cross(Right).Normalized();

	FMatrix RotMatrix;
	RotMatrix.SetAxes(Forward, Right, Up);

	return FMatrix::MakeScaleMatrix(GetWorldScale()) * RotMatrix * FMatrix::MakeTranslationMatrix(GetWorldLocation());
}
