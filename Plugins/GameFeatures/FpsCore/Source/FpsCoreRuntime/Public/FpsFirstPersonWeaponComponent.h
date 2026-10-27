// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "FpsFirstPersonWeaponComponent.generated.h"

class AActor;
class AController;
class APawn;
class UFpsFirstPersonAnimInstance;
class ULyraEquipmentManagerComponent;
class ULyraPawnComponent_CharacterParts;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
struct FPropertyChangedEvent;

/**
 * Lyra equips weapons on Character::GetMesh() (CharacterMesh0). Local 1P view uses
 * cosmetic FirstPersonMesh (OnlyOwnerSee) while MeshComponent is OwnerNoSee — so the
 * replicated gun is invisible to the owning player. This component hides the
 * replicated gun from the owner and spawns a non-replicated visual on FirstPersonMesh.
 *
 * The owner's third-person body stays out of the owner's main pass but keeps casting shadows,
 * so the player sees their own full-body shadow (the weapons cast none for the owner).
 *
 * If the visual has a UFpsWeaponGripComponent, its profile and mesh are handed to the
 * 1P UFpsFirstPersonAnimInstance, which solves the two-handed grip every frame.
 *
 * Event driven (no polling): possession, equipment changes (incl. the weapon actors replicating
 * and attaching on clients), cosmetic part changes, the 1P anim instance being (re)initialized and
 * the grip getting locked each request one sync on the next tick.
 */
UCLASS(ClassGroup = (FPS), meta = (BlueprintSpawnableComponent))
class UFpsFirstPersonWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFpsFirstPersonWeaponComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SyncFirstPersonWeaponVisuals();

	/** Coalesces events into one SyncFirstPersonWeaponVisuals on the next tick. */
	void RequestSync();

protected:

	/** Binds whichever sync sources exist and are not bound yet; components can be added after BeginPlay. */
	void BindPawnSources(APawn* Pawn);
	void BindFirstPersonMesh(USkeletalMeshComponent* FirstPersonMesh);
	/** Watches the replicated weapons for removal, and those not attached yet for their (replicated) attachment. */
	void WatchReplicatedWeapons(const TArray<AActor*>& ReplicatedWeapons);
	void UnbindAll();

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);
	UFUNCTION()
	void HandleCharacterPartsChanged(ULyraPawnComponent_CharacterParts* ComponentWithChangedParts);
	UFUNCTION()
	void HandleFirstPersonAnimInitialized();
	UFUNCTION()
	void HandleWeaponEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	void HandleEquipmentChanged(ULyraEquipmentManagerComponent* EquipmentManager);
	void HandleWeaponTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
#if WITH_EDITOR
	void HandleSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent);
#endif

	void ClearLocalVisuals();
	USkeletalMeshComponent* FindFirstPersonMesh() const;

	/**
	 * Pass the visual's grip profile (or none) to the first-person anim instance, and once it has
	 * locked the grip, hang the visual off the primary hand with that relation (see
	 * UFpsFirstPersonAnimInstance::GetLockedGrip) so locomotion cannot slide it in the hand.
	 */
	void ApplyWeaponGrip(AActor* LocalWeaponVisual);

	/** Hide a third-person primitive from the owner but keep its shadow (see bOwnerSeesThirdPersonShadow). */
	void HideFromOwnerKeepShadow(UPrimitiveComponent* Primitive) const;
	/** Returns the owner's third-person body mesh (null if none found). */
	USkeletalMeshComponent* ApplyOwnerBodyShadow(const USkeletalMeshComponent* FirstPersonMesh) const;

	/**
	 * Engine first person rendering (UFpsFirstPersonSettings::bFirstPersonRendering): the 1P body is a
	 * First Person primitive with its legs hidden; a world layer copy of it (same mesh, the 3P body's
	 * copy-pose anim class reading its attach parent) draws only the legs.
	 */
	void UpdateFirstPersonLayers(USkeletalMeshComponent* FirstPersonMesh, const USkeletalMeshComponent* ThirdPersonBody);
	void DestroyWorldLayerLegs();

	/** The owner sees the shadow of their third-person body (the 1P meshes and weapons cast none). */
	UPROPERTY(EditAnywhere, Category = "FPS")
	bool bOwnerSeesThirdPersonShadow = true;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> LocalWeaponVisuals;

	/** Per visual: the socket and relative transform it copies from the replicated weapon. */
	TArray<FName> LocalWeaponSockets;
	TArray<FTransform> LocalWeaponRelative;

	/** Locked grip version the first visual is currently attached with (INDEX_NONE: on its socket). */
	int32 AppliedGripVersion = INDEX_NONE;
	TWeakObjectPtr<AActor> AppliedGripVisual;

	TWeakObjectPtr<USkeletalMeshComponent> CachedFirstPersonMesh;

	/** World layer legs (owner only), see UpdateFirstPersonLayers. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> WorldLayerLegs;

	int32 LastSyncSignature = 0;
	bool bSyncRequested = false;

	TWeakObjectPtr<ULyraEquipmentManagerComponent> BoundEquipmentManager;
	FDelegateHandle EquipmentChangedHandle;
	TWeakObjectPtr<ULyraPawnComponent_CharacterParts> BoundCharacterParts;
	TWeakObjectPtr<USkeletalMeshComponent> BoundFirstPersonMesh;
	TWeakObjectPtr<UFpsFirstPersonAnimInstance> BoundAnimInstance;
	FDelegateHandle GripLockedHandle;
	TArray<TWeakObjectPtr<AActor>> WatchedWeapons;
	/** Root components of replicated weapons still waiting for their attachment to replicate. */
	TArray<TPair<TWeakObjectPtr<USceneComponent>, FDelegateHandle>> PendingAttachRoots;
#if WITH_EDITOR
	FDelegateHandle SettingsChangedHandle;
#endif
};
