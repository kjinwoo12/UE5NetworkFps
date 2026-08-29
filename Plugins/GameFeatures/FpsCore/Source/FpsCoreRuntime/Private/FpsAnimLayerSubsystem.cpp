// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsAnimLayerSubsystem.h"

#include "Cosmetics/LyraPawnComponent_CharacterParts.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FpsAnimLayerComponent.h"
#include "FpsFirstPersonWeaponComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsAnimLayerSubsystem)

bool UFpsAnimLayerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UFpsAnimLayerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleActorSpawned));

	for (TActorIterator<ACharacter> It(&InWorld); It; ++It)
	{
		TryAddAnimLayerComponent(*It);
	}
}

void UFpsAnimLayerSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (ActorSpawnedHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
			ActorSpawnedHandle.Reset();
		}
	}

	Super::Deinitialize();
}

void UFpsAnimLayerSubsystem::HandleActorSpawned(AActor* SpawnedActor)
{
	TryAddAnimLayerComponent(Cast<ACharacter>(SpawnedActor));
}

void UFpsAnimLayerSubsystem::TryAddAnimLayerComponent(ACharacter* Character)
{
	if (!Character)
	{
		return;
	}

	// Only pawns that use Lyra cosmetics (FpsCore / Shooter mannequin heroes).
	if (!Character->FindComponentByClass<ULyraPawnComponent_CharacterParts>())
	{
		return;
	}

	if (!Character->FindComponentByClass<UFpsAnimLayerComponent>())
	{
		UFpsAnimLayerComponent* NewComp = NewObject<UFpsAnimLayerComponent>(Character, TEXT("FpsAnimLayer"));
		NewComp->RegisterComponent();
	}

	if (!Character->FindComponentByClass<UFpsFirstPersonWeaponComponent>())
	{
		UFpsFirstPersonWeaponComponent* WeaponView =
			NewObject<UFpsFirstPersonWeaponComponent>(Character, TEXT("FpsFirstPersonWeapon"));
		WeaponView->RegisterComponent();
	}
}
