// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "FpsWeaponGripProfile.generated.h"

/**
 * How the first-person view re-places one weapon's hold. One asset per weapon, referenced by
 * UFpsWeaponGripComponent on the weapon actor; applied by UFpsFirstPersonAnimInstance.
 *
 * The grip itself (support hand position, wrist angle, fingers) comes from the weapon's grip
 * pose animation, kept relative to the primary hand. The profile places the weapon for the 1P
 * view and can turn the support grip around the barrel (side hold -> underneath).
 *
 * View space: X forward, Y right, Z up (cm), relative to the player camera.
 */
UCLASS(BlueprintType)
class UFpsWeaponGripProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/**
	 * Weapon attach bone (weapon_r) location in view space, i.e. where the weapon sits on screen.
	 * The weapon bone, not the hand, is pinned: locomotion animations move weapon_r inside the
	 * hand, so pinning the hand would swing the weapon. The hand follows the animation's relation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector WeaponViewOffset = FVector::ZeroVector;

	/** Weapon attach bone rotation in view space; set it so the barrel axis is level with view X. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FRotator WeaponViewRotation = FRotator::ZeroRotator;

	/**
	 * WeaponViewOffset/Rotation for true first person (UFpsFirstPersonSettings), where the shoulders
	 * are the animated ones. An arm that cannot reach turns its clavicle forward (ReachArmFraction),
	 * so this can sit further forward than the idle animation holds weapon_r. The two above stay for
	 * the legacy rig.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|True First Person")
	FVector TrueFirstPersonWeaponViewOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|True First Person")
	FRotator TrueFirstPersonWeaponViewRotation = FRotator::ZeroRotator;

	FVector GetWeaponViewOffset(bool bTrueFirstPerson) const { return bTrueFirstPerson ? TrueFirstPersonWeaponViewOffset : WeaponViewOffset; }
	FRotator GetWeaponViewRotation(bool bTrueFirstPerson) const { return bTrueFirstPerson ? TrueFirstPersonWeaponViewRotation : WeaponViewRotation; }

	/** Keep the support hand where the grip animation puts it relative to the primary hand. Off for one-handed weapons. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Support Hand")
	bool bSupportHandFollowsAnimation = true;

	/**
	 * Turns the animated support grip around the barrel (view X through SupportGripPivotViewOffset),
	 * e.g. from the side of the handguard to underneath it. 0 = as animated. Degrees, + rolls the
	 * hand from the left side toward the bottom.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Support Hand")
	float SupportGripRollDegrees = 0.0f;

	/**
	 * A point on the barrel axis in view space (only Y and Z matter), for WeaponViewOffset. With the
	 * true first person placement it moves along with the weapon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Support Hand")
	FVector SupportGripPivotViewOffset = FVector::ZeroVector;
};
