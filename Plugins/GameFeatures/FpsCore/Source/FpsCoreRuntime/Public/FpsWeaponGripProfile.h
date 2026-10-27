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

	/**
	 * Aiming down sights (UFpsFirstPersonSettings::AimTag on the pawn's ASC, e.g. GA_Fps_ADS): the
	 * weapon bone moves from the hip placement above to this view transform. Set it so the sight
	 * line lies on view X (screen center), the rear sight AimWeaponViewOffset.X + its distance from
	 * weapon_r in front of the eye. Both rigs use it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim", meta = (EditCondition = "bCanAim"))
	FVector AimWeaponViewOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim", meta = (EditCondition = "bCanAim"))
	FRotator AimWeaponViewRotation = FRotator::ZeroRotator;

	/** Off: the weapon stays at the hip and the view does not zoom while aiming (the movement and input changes of the ability still apply). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim")
	bool bCanAim = false;

	/** Time from hip to sights and back (s). The camera zoom and the weapon move use the same blend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim", meta = (EditCondition = "bCanAim", ClampMin = "0.0", Units = "s"))
	float AimBlendTime = 0.2f;

	/** Camera field of view while aiming, as a fraction of the camera mode's (1 = no zoom). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim", meta = (EditCondition = "bCanAim", ClampMin = "0.1", ClampMax = "1.0"))
	float AimFieldOfViewScale = 0.8f;

	/** Weapon sway and turn lag while aiming, as a fraction of the hip values: the sight has to stay on the screen center. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aim", meta = (EditCondition = "bCanAim", ClampMin = "0.0", ClampMax = "1.0"))
	float AimWeaponSwayScale = 0.25f;

	/** Weapon bone transform in view space for an aim blend (0 hip .. 1 sights). */
	FTransform GetWeaponViewTransform(bool bTrueFirstPerson, float AimAlpha) const
	{
		const FTransform Hip(GetWeaponViewRotation(bTrueFirstPerson), GetWeaponViewOffset(bTrueFirstPerson));
		if (!bCanAim || AimAlpha <= 0.0f)
		{
			return Hip;
		}
		const FTransform Aim(AimWeaponViewRotation, AimWeaponViewOffset);
		FTransform Blended;
		Blended.Blend(Hip, Aim, FMath::Min(AimAlpha, 1.0f));
		return Blended;
	}

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
