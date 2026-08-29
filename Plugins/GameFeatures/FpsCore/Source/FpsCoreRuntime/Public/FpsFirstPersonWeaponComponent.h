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
 * Left-hand TwoBone IK (default on) snaps the support hand to Hand_L / Grip_L on the
 * local weapon mesh. Right-hand IK stays off — CopyPose + weapon linked layers handle it.
 */
UCLASS(ClassGroup = (FPS), meta = (BlueprintSpawnableComponent))
class UFpsFirstPersonWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFpsFirstPersonWeaponComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SyncFirstPersonWeaponVisuals();

protected:

	void ClearLocalVisuals();
	USkeletalMeshComponent* FindFirstPersonMesh() const;
	void EnsureTickPrerequisites(USkeletalMeshComponent* FirstPersonMesh);

	bool ShouldApplyLeftHandIk(AActor* LocalWeaponVisual) const;
	bool ShouldApplyRightHandIk(AActor* LocalWeaponVisual);
	bool HasActiveWeaponAnimLayer();

	void ApplyWeaponHandIk(bool bEnableLeftHandIk, bool bEnableRightHandIk, AActor* LocalWeaponVisual);

	static void SetAnimInstanceBool(UAnimInstance* AnimInstance, FName PropertyName, bool bValue);
	static void SetAnimInstanceVector(UAnimInstance* AnimInstance, FName PropertyName, const FVector& Value);

	UPROPERTY(EditAnywhere, Category = "FPS|HandIK")
	FName RightHandSocketName = TEXT("Hand_R");

	UPROPERTY(EditAnywhere, Category = "FPS|HandIK")
	FName LeftHandSocketName = TEXT("Hand_L");

	/** Snap the support hand to weapon Hand_L / Grip_L (recommended default). */
	UPROPERTY(EditAnywhere, Category = "FPS|HandIK")
	bool bApplyLeftHandIk = true;

	/**
	 * Off by default — CopyPose + weapon linked layers already grip with the right hand.
	 * Enabling both hands often over-stretches the right arm.
	 */
	UPROPERTY(EditAnywhere, Category = "FPS|HandIK")
	bool bApplyRightHandIk = false;

	/** When bApplyRightHandIk is true, skip right IK while a weapon anim layer is active. */
	UPROPERTY(EditAnywhere, Category = "FPS|HandIK", meta = (EditCondition = "bApplyRightHandIk"))
	bool bSkipRightHandIkWhenWeaponAnimLayerActive = true;

	/** Used when the weapon mesh has no Hand_L / Grip_L (e.g. pistol with only Grip). Weapon-local space. */
	UPROPERTY(EditAnywhere, Category = "FPS|HandIK", meta = (EditCondition = "bApplyLeftHandIk"))
	FVector LeftHandGripFallbackOffset = FVector(-2.f, 8.f, -1.f);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> LocalWeaponVisuals;

	int32 LastSyncSignature = 0;
	bool bLastAnyHandIkEnabled = false;
	bool bLastDisableRightHandIk = true;
	bool bLastDisableLeftHandIk = false;
	bool bCachedTickPrerequisites = false;

	FVector LastEffectorLocation_R = FVector::ZeroVector;
	FVector LastEffectorLocation_L = FVector::ZeroVector;
	FVector LastJointTarget_R = FVector::ZeroVector;
	FVector LastJointTarget_L = FVector::ZeroVector;
};
