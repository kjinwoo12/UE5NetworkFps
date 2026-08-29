// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "UObject/SoftObjectPtr.h"

#include "FpsLoadoutComponent.generated.h"

class ULyraEquipmentDefinition;

/**
 * Authority-only hero loadout: equips Lyra equipment after ASC is ready (no QuickBar).
 * Set EquipmentToGrant per hero BP (e.g. Soldier → WID_Rifle).
 *
 * Timing matches QuickBar better than raw BeginPlay: wait for Ability System, then
 * grant on the next tick so FpsAnimLayer unarmed link can run first; weapon
 * OnEquipped then applies rifle anim layers.
 */
UCLASS(Blueprintable, ClassGroup = (FPS), meta = (BlueprintSpawnableComponent))
class UFpsLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFpsLoadoutComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void BeginPlay() override;

	void HandleAbilitySystemInitialized();
	void GrantLoadoutEquipment();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TArray<TSoftClassPtr<ULyraEquipmentDefinition>> EquipmentToGrant;

	bool bLoadoutGranted = false;
	int32 LoadoutGrantRetryCount = 0;
};
