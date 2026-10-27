// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCameraMode_FirstPerson.h"

#include "Camera/LyraCameraComponent.h"
#include "FpsFirstPersonSettings.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCameraMode_FirstPerson)

ULyraCameraMode_FirstPerson::ULyraCameraMode_FirstPerson()
{
	BlendTime = 0.25f;
}

void ULyraCameraMode_FirstPerson::UpdateView(float DeltaTime)
{
	UpdateForTarget(DeltaTime);
	UpdateCrouchOffset(DeltaTime);

	FRotator PivotRotation = GetPivotRotation();
	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	// True first person: eyes on the animated head (crouch, lean and look-down come from the animation).
	FVector PivotLocation = GetPivotLocation() + CurrentCrouchOffset;
	UFpsFirstPersonSettings::GetTrueFirstPersonEyeLocation(Cast<APawn>(GetTargetActor()), PivotRotation, PivotLocation);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;

	ApplyFirstPersonRendering(GetDefault<UFpsFirstPersonSettings>()->UseFirstPersonRendering());
}

void ULyraCameraMode_FirstPerson::OnDeactivation()
{
	Super::OnDeactivation();
	ApplyFirstPersonRendering(false);
}

void ULyraCameraMode_FirstPerson::ApplyFirstPersonRendering(bool bEnable) const
{
	// Read by ULyraCameraComponent::GetCameraView into the view's first person parameters.
	if (ULyraCameraComponent* CameraComponent = GetLyraCameraComponent())
	{
		const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
		CameraComponent->SetEnableFirstPersonScale(bEnable);
		CameraComponent->SetFirstPersonScale(Settings->FirstPersonScale);
		CameraComponent->SetEnableFirstPersonFieldOfView(bEnable && Settings->FirstPersonFieldOfView > 0.0f);
		CameraComponent->SetFirstPersonFieldOfView(Settings->FirstPersonFieldOfView > 0.0f ? Settings->FirstPersonFieldOfView : FieldOfView);
	}
}

void ULyraCameraMode_FirstPerson::UpdateForTarget(float DeltaTime)
{
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		if (TargetCharacter->IsCrouched())
		{
			const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
			const float CrouchedHeightAdjustment = TargetCharacterCDO->CrouchedEyeHeight - TargetCharacterCDO->BaseEyeHeight;

			SetTargetCrouchOffset(FVector(0.f, 0.f, CrouchedHeightAdjustment));
			return;
		}
	}

	SetTargetCrouchOffset(FVector::ZeroVector);
}

void ULyraCameraMode_FirstPerson::SetTargetCrouchOffset(FVector NewTargetOffset)
{
	CrouchOffsetBlendPct = 0.0f;
	InitialCrouchOffset = CurrentCrouchOffset;
	TargetCrouchOffset = NewTargetOffset;
}

void ULyraCameraMode_FirstPerson::UpdateCrouchOffset(float DeltaTime)
{
	if (CrouchOffsetBlendPct < 1.0f)
	{
		CrouchOffsetBlendPct = FMath::Min(CrouchOffsetBlendPct + DeltaTime * CrouchOffsetBlendMultiplier, 1.0f);
		CurrentCrouchOffset = FMath::InterpEaseInOut(InitialCrouchOffset, TargetCrouchOffset, CrouchOffsetBlendPct, 1.0f);
	}
	else
	{
		CurrentCrouchOffset = TargetCrouchOffset;
		CrouchOffsetBlendPct = 1.0f;
	}
}
