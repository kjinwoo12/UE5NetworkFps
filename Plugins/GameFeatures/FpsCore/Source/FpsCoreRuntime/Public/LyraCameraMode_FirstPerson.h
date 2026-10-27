// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/LyraCameraMode.h"

#include "LyraCameraMode_FirstPerson.generated.h"

/**
 * ULyraCameraMode_FirstPerson
 *
 *	First-person camera mode: control rotation, located at the animated head in true first person
 *	(UFpsFirstPersonSettings), otherwise at Lyra's eye-height pivot.
 */
UCLASS(Blueprintable)
class ULyraCameraMode_FirstPerson : public ULyraCameraMode
{
	GENERATED_BODY()

public:

	ULyraCameraMode_FirstPerson();

protected:

	virtual void UpdateView(float DeltaTime) override;
	virtual void OnDeactivation() override;

	/** Engine first person rendering parameters on the camera (UFpsFirstPersonSettings::bFirstPersonRendering). */
	void ApplyFirstPersonRendering(bool bEnable) const;

	void UpdateForTarget(float DeltaTime);
	void SetTargetCrouchOffset(FVector NewTargetOffset);
	void UpdateCrouchOffset(float DeltaTime);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "First Person")
	float CrouchOffsetBlendMultiplier = 5.0f;

	FVector InitialCrouchOffset = FVector::ZeroVector;
	FVector TargetCrouchOffset = FVector::ZeroVector;
	float CrouchOffsetBlendPct = 1.0f;
	FVector CurrentCrouchOffset = FVector::ZeroVector;
};
