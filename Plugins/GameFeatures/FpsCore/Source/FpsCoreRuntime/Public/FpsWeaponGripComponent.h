// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "FpsWeaponGripComponent.generated.h"

class UFpsWeaponGripProfile;

/**
 * Put on a weapon actor to re-place its hold for the first-person view.
 * The 1P weapon visual (UFpsFirstPersonWeaponComponent) reads it and hands the profile to
 * UFpsFirstPersonAnimInstance. Weapons without it are held by animation only.
 */
UCLASS(ClassGroup = (FPS), meta = (BlueprintSpawnableComponent))
class UFpsWeaponGripComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFpsWeaponGripComponent();

	const UFpsWeaponGripProfile* GetGripProfile() const { return GripProfile; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grip")
	TObjectPtr<const UFpsWeaponGripProfile> GripProfile;
};
