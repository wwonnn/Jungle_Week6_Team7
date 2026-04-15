#include "RotatingMovementComponent.h"

#include "Object/ObjectFactory.h"
#include "SceneComponent.h"
#include "Serialization/Archive.h"

IMPLEMENT_CLASS(URotatingMovementComponent, UMovementComponent)

void URotatingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
	UMovementComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USceneComponent* UpdatedSceneComponent = GetUpdatedComponent();
	if (!UpdatedSceneComponent || (RotationRate.IsNearlyZero() && PivotTranslation.Length() < EPSILON))
	{
		return;
	}

	FQuat DeltaQuat = (RotationRate * DeltaTime).ToQuaternion();

	if (PivotTranslation.Length() < EPSILON)
	{
		// 단순 자전
		if (bRotationInLocalSpace)
		{
			UpdatedSceneComponent->AddLocalRotation(DeltaQuat);
		}
		else
		{
			// 월드 축 기준 회전
			FQuat NewQuat = DeltaQuat * UpdatedSceneComponent->GetRelativeQuat();
			UpdatedSceneComponent->SetRelativeRotation(NewQuat);
		}
	}
	else
	{
		// 피벗 기준 공전 + 자전
		FVector RelativeLocation = UpdatedSceneComponent->GetRelativeLocation();
		FQuat RelativeQuat = UpdatedSceneComponent->GetRelativeQuat();

		FVector ToObject = RelativeLocation - PivotTranslation;
		FVector RotatedOffset = DeltaQuat.RotateVector(ToObject);
		FVector NewLocation = PivotTranslation + RotatedOffset;

		FQuat NewQuat = DeltaQuat * RelativeQuat;

		UpdatedSceneComponent->SetRelativeLocation(NewLocation);
		UpdatedSceneComponent->SetRelativeRotation(NewQuat);
	}
}

void URotatingMovementComponent::Serialize(FArchive& Ar)
{
	UMovementComponent::Serialize(Ar);
	Ar << RotationRate.Pitch;
	Ar << RotationRate.Yaw;
	Ar << RotationRate.Roll;
	Ar << PivotTranslation.X;
	Ar << PivotTranslation.Y;
	Ar << PivotTranslation.Z;
	Ar << bRotationInLocalSpace;
}

void URotatingMovementComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UMovementComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "Rotation Rate", EPropertyType::Rotator, &RotationRate, 0.0f, 0.0f, 0.1f });
	OutProps.push_back({ "Pivot Translation", EPropertyType::Vec3, &PivotTranslation, 0.0f, 0.0f, 0.1f });
	OutProps.push_back({ "Rotation In Local Space", EPropertyType::Bool, &bRotationInLocalSpace, 0.0f, 0.0f, 0.1f });
}
