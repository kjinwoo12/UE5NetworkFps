// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsAim.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "FpsFirstPersonSettings.h"
#include "FpsWeaponGripComponent.h"
#include "FpsWeaponGripProfile.h"
#include "GameFramework/Pawn.h"

bool FpsAim::IsAiming(const APawn* Pawn)
{
	const FGameplayTag& AimTag = GetDefault<UFpsFirstPersonSettings>()->AimTag;
	const UAbilitySystemComponent* AbilitySystem = Pawn && AimTag.IsValid() ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn) : nullptr;
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AimTag);
}

const UFpsWeaponGripProfile* FpsAim::FindHeldGripProfile(const APawn* Pawn)
{
	if (!Pawn)
	{
		return nullptr;
	}

	// The equipped weapon actor hangs off the pawn's mesh (the 1P copy off the cosmetic body below it).
	TArray<AActor*> AttachedActors;
	Pawn->GetAttachedActors(AttachedActors, true, true);
	for (const AActor* Actor : AttachedActors)
	{
		const UFpsWeaponGripComponent* Grip = Actor ? Actor->FindComponentByClass<UFpsWeaponGripComponent>() : nullptr;
		if (Grip && Grip->GetGripProfile())
		{
			return Grip->GetGripProfile();
		}
	}
	return nullptr;
}

void FFpsAimBlend::Update(bool bAiming, const UFpsWeaponGripProfile* Profile, float DeltaSeconds)
{
	const bool bTarget = bAiming && Profile && Profile->bCanAim;
	const float BlendTime = Profile ? Profile->AimBlendTime : 0.0f;
	Linear = BlendTime > 0.0f
		? FMath::FInterpConstantTo(Linear, bTarget ? 1.0f : 0.0f, DeltaSeconds, 1.0f / BlendTime)
		: (bTarget ? 1.0f : 0.0f);
}

float FFpsAimBlend::GetAlpha() const
{
	return FMath::SmoothStep(0.0f, 1.0f, Linear);
}
