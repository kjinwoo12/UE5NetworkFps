// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"

#include "FpsAnimLayerSubsystem.generated.h"

class AActor;
class ACharacter;

/**
 * While FpsCoreRuntime is loaded, ensures FPS pawns get UFpsAnimLayerComponent
 * without requiring a Blueprint SCS edit.
 */
UCLASS()
class UFpsAnimLayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void HandleActorSpawned(AActor* SpawnedActor);
	void TryAddAnimLayerComponent(ACharacter* Character);

	FDelegateHandle ActorSpawnedHandle;
};
