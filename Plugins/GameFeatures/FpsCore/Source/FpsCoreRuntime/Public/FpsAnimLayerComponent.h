// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Cosmetics/LyraCosmeticAnimationTypes.h"
#include "Engine/EngineTypes.h"

#include "FpsAnimLayerComponent.generated.h"

class ULyraPawnComponent_CharacterParts;
class USkeletalMeshComponent;

/**
 * Re-links default (unarmed) anim layers on CharacterMesh0 after cosmetic parts change.
 * Lyra CharacterParts SetSkeletalMesh(bReinitPose) recreates the AnimInstance and wipes
 * LinkAnimClassLayers — so re-link here on the driving mesh only.
 *
 * Cosmetic part meshes:
 *   - MeshComponent (3P)     → ABP_Mannequin_CopyPose (copies CharacterMesh0)
 *   - FirstPersonMesh (1P)   → ABP_Mannequin_FirstPersonCopy (CopyPose + ControlRig)
 * Neither should receive LinkAnimClassLayers.
 * Also SetOwner(pawn) on part child actors so OwnerNoSee / OnlyOwnerSee see the ViewTarget.
 * Weapon equipment still replaces layers via B_WeaponInstance_Base::OnEquipped.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UFpsAnimLayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFpsAnimLayerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Animation|ThirdPerson")
	FLyraAnimLayerSelectionSet ThirdPersonUnarmedLayers;

	UFUNCTION()
	void HandleCharacterPartsChanged(ULyraPawnComponent_CharacterParts* PartsComponent);

	void BindToCosmeticsComponents();
	void SyncCosmeticPartOwners();
	void ScheduleLinkDefaultAnimLayers();
	void LinkDefaultAnimLayers();

	static void LinkLayersOnMesh(USkeletalMeshComponent* Mesh, TSubclassOf<UAnimInstance> LayerClass);

	TArray<TWeakObjectPtr<ULyraPawnComponent_CharacterParts>> BoundCosmeticsComponents;
	FTimerHandle DeferredLinkTimerHandle;
};
