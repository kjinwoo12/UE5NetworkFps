// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "FpsFirstPersonWeaponComponent.generated.h"

class AActor;
class UAnimInstance;
class USkeletalMeshComponent;

/**
 * Lyra equips weapons on Character::GetMesh() (CharacterMesh0). Local 1P view uses
 * cosmetic FirstPersonMesh (OnlyOwnerSee) while MeshComponent is OwnerNoSee — so the
 * replicated gun is invisible to the owning player. This component hides the
 * replicated gun from the owner and spawns a non-replicated visual on FirstPersonMesh.
 *
 * Also drives FirstPersonCopy TwoBoneIK effectors (Hand_R / Hand_L sockets on the
 * local weapon mesh) so hands snap to the rifle grips.
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

protected:

	void ClearLocalVisuals();
	USkeletalMeshComponent* FindFirstPersonMesh() const;

	/** Enable/disable ABP weapon hand IK and push effector locations in mesh space. */
	void ApplyWeaponHandIk(bool bEnableHandIk, AActor* LocalWeaponVisual) const;

	static void SetAnimInstanceBool(UAnimInstance* AnimInstance, FName PropertyName, bool bValue);
	static void SetAnimInstanceVector(UAnimInstance* AnimInstance, FName PropertyName, const FVector& Value);

	/** Preferred grip sockets on SK_Rifle (fallback order applied in code). */
	UPROPERTY(EditAnywhere, Category = "FPS|HandIK")
	FName RightHandSocketName = TEXT("Hand_R");

	UPROPERTY(EditAnywhere, Category = "FPS|HandIK")
	FName LeftHandSocketName = TEXT("Hand_L");

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> LocalWeaponVisuals;

	FTimerHandle SyncTimerHandle;
	int32 LastSyncSignature = 0;
	bool bLastHandIkEnabled = false;
};
