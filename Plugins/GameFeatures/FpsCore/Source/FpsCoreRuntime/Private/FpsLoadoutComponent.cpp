// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsLoadoutComponent.h"

#include "Character/LyraPawnExtensionComponent.h"
#include "Equipment/LyraEquipmentDefinition.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Engine/World.h"
#include "FpsAnimLayerComponent.h"
#include "FpsFirstPersonWeaponComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsLoadoutComponent)

UFpsLoadoutComponent::UFpsLoadoutComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UFpsLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || EquipmentToGrant.Num() == 0)
	{
		return;
	}

	if (ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Owner))
	{
		PawnExt->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemInitialized));
	}
	else
	{
		HandleAbilitySystemInitialized();
	}
}

void UFpsLoadoutComponent::HandleAbilitySystemInitialized()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		GrantLoadoutEquipment();
		return;
	}

	// Next tick: FpsAnimLayerComponent often links unarmed layers on the same tick.
	// Grant after that so B_WeaponInstance_* OnEquipped rifle layers win (QuickBar-like order).
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ThisClass::GrantLoadoutEquipment));
}

void UFpsLoadoutComponent::GrantLoadoutEquipment()
{
	if (bLoadoutGranted)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || EquipmentToGrant.Num() == 0)
	{
		return;
	}

	ULyraEquipmentManagerComponent* EquipmentManager = Owner->FindComponentByClass<ULyraEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		// ShooterCore may still be wiring components; retry a few times.
		if (LoadoutGrantRetryCount < 10)
		{
			++LoadoutGrantRetryCount;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateUObject(this, &ThisClass::GrantLoadoutEquipment));
			}
		}
		return;
	}

	bLoadoutGranted = true;

	for (const TSoftClassPtr<ULyraEquipmentDefinition>& SoftEquip : EquipmentToGrant)
	{
		if (SoftEquip.IsNull())
		{
			continue;
		}

		if (TSubclassOf<ULyraEquipmentDefinition> EquipDef = SoftEquip.LoadSynchronous())
		{
			EquipmentManager->EquipItem(EquipDef);
		}
	}

	// Cosmetics / deferred unarmed link may wipe layers; weapon BP GetTypedPawn may
	// also fail on non-ShooterMannequin heroes. Force FpsAnimLayer to pick weapon layers.
	if (UFpsAnimLayerComponent* AnimLayers = Owner->FindComponentByClass<UFpsAnimLayerComponent>())
	{
		AnimLayers->RefreshAnimLayers();
	}

	if (UFpsFirstPersonWeaponComponent* FPWeapons = Owner->FindComponentByClass<UFpsFirstPersonWeaponComponent>())
	{
		FPWeapons->SyncFirstPersonWeaponVisuals();
	}
}
