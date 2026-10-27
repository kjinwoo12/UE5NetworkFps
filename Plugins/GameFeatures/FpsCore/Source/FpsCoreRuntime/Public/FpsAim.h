// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class APawn;
class UFpsWeaponGripProfile;

/**
 * Aiming down sights for the first-person view.
 *
 * The aim state is UFpsFirstPersonSettings::AimTag on the pawn's ability system (granted while the
 * ADS ability is active; activation owned tags replicate, so the third-person ABP of other players
 * sees it as well). The owner's camera (ULyraCameraMode_FirstPerson, zoom) and the 1P body
 * (UFpsFirstPersonAnimInstance, weapon to the sights) each run an FFpsAimBlend with the held
 * weapon's UFpsWeaponGripProfile, so both move together.
 */
namespace FpsAim
{
	bool IsAiming(const APawn* Pawn);

	/** Grip profile of the weapon actor attached to the pawn (UFpsWeaponGripComponent), or null. */
	const UFpsWeaponGripProfile* FindHeldGripProfile(const APawn* Pawn);
}

/** Hip (0) to sights (1) blend over the profile's AimBlendTime, eased at both ends. */
struct FFpsAimBlend
{
	void Update(bool bAiming, const UFpsWeaponGripProfile* Profile, float DeltaSeconds);
	void Reset() { Linear = 0.0f; }

	/** Eased alpha. */
	float GetAlpha() const;

private:
	float Linear = 0.0f;
};
