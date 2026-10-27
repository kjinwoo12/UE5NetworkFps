// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "FpsAim.h"

#include "FpsFirstPersonAnimInstance.generated.h"

class APawn;
class UControlRig;
class UFpsWeaponGripProfile;
class USkeletalMeshComponent;

/**
 * Native parent for the owner-only first-person body ABP (ABP_Mannequin_FirstPersonCopy).
 *
 * The 1P mesh lives on a cosmetic child actor, so TryGetPawnOwner() fails; the pawn is
 * found through the actor owner / attach chain instead.
 *
 * True first person (UFpsFirstPersonSettings, default): the pose is the third-person animation
 * copied from the pawn's mesh, the camera sits on its head, and only the arms are re-solved.
 * Legacy: AimPitch is pushed into the first-person Control Rig (CR_Mannequin_FirstPerson), which
 * pins the head to the camera and rotates the shoulder anchors with pitch. RigAlpha switches it.
 *
 * Two-handed weapon grip (see UFpsWeaponGripProfile) is a position correction only:
 * the weapon attach bone (weapon_r) is pinned to a view-relative transform, the primary hand keeps
 * the animation's relation to it, and the
 * support hand keeps the transform the grip animation gives it relative to the primary hand
 * (read from the pawn's animated mesh), so it holds the weapon exactly as in third person.
 * Elbow poles blend between the animated elbow and the one that keeps the wrist straight.
 * Solved from this frame's camera, so nothing trails a frame behind.
 */
UCLASS(Blueprintable, Config = Game)
class UFpsFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "First Person")
	APawn* GetFirstPersonPawn() const;

	/** Set by UFpsFirstPersonWeaponComponent; null turns the grip IK off. */
	void SetWeaponGrip(const UFpsWeaponGripProfile* InProfile);

	/**
	 * Locomotion animations slide the weapon bone inside the primary hand (5-9cm walking backwards),
	 * which reads as hands sliding on a view-locked weapon. Once the pawn has stood still for
	 * GripCaptureSettleSeconds the weapon-in-hand and support-hand-on-weapon relations are captured
	 * and used from then on. The weapon component attaches the 1P weapon visual to the primary hand
	 * with OutWeaponInPrimaryHand and confirms the version; until then the live relation is used.
	 */
	bool GetLockedGrip(FTransform& OutWeaponInPrimaryHand, int32& OutVersion) const;
	void ConfirmLockedGripAttached(int32 Version) { AttachedGripVersion = Version; }
	FName GetPrimaryHandBone() const { return PrimaryHandBone; }
	FName GetWeaponBone() const { return WeaponBone; }

	/** Fires (game thread, mid animation update) when a grip gets locked; GetLockedGrip has the new version. */
	FSimpleMulticastDelegate OnGripLocked;

protected:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	/** Captures the finger pose (see UFpsFirstPersonSettings::LockedFingerRoots) and blends it in or out. */
	void UpdateFingerLock(const USkeletalMeshComponent* SourceMesh, float DeltaSeconds);

	void ApplyHiddenBones();
	void UpdateWeaponGrip(const APawn* Pawn, float DeltaSeconds);
	void UpdateGripCapture(const APawn* Pawn, const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, float DeltaSeconds);
	FVector ShoulderLineInView(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld) const;
	FVector ShoulderLineInHeading(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld) const;
	void UpdateChestCorrection(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, float DeltaSeconds);
	const USkeletalMeshComponent* FindGripSourceMesh(const APawn* Pawn) const;
	FVector StraightWristElbowTarget(const FTransform& HandWorld, FName HandBone) const;
	void SetBoolVariable(FName VariableName, bool bValue);
	void SetVectorVariable(FName VariableName, const FVector& Value);
	void PushControlRigVariables();
	UControlRig* FindFirstPersonControlRig() const;

protected:

	/** View pitch in degrees, -90 (down) .. 90 (up). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person")
	float AimPitch = 0.0f;

	/** Hand rotations in mesh component space, applied after the hand IK (Transform Bone nodes). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Grip")
	FRotator RightHandRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Grip")
	FRotator LeftHandRotation = FRotator::ZeroRotator;

	/** Bones hidden on the owner-only mesh, e.g. the head that sits on the camera. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "First Person")
	TArray<FName> HiddenBoneNames;

	/** Control Rig public float variable that receives AimPitch. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FName ControlRigPitchVariable = TEXT("AimPitch");

	/** Control Rig public float variable for the rig's weight: 1 legacy, 0 true first person (UFpsFirstPersonSettings). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FName ControlRigAlphaVariable = TEXT("RigAlpha");

	/** Hand bones. The support hand keeps its animated pose relative to the primary hand; elbows are aimed so each wrist stays straight. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName PrimaryHandBone = TEXT("hand_r");

	/** Weapon attach bone under the primary hand; pinned to the profile's view transform. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName WeaponBone = TEXT("weapon_r");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName SupportHandBone = TEXT("hand_l");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName PrimaryLowerArmBone = TEXT("lowerarm_r");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName SupportLowerArmBone = TEXT("lowerarm_l");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName PrimaryClavicleBone = TEXT("clavicle_r");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName SupportClavicleBone = TEXT("clavicle_l");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName PrimaryUpperArmBone = TEXT("upperarm_r");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName SupportUpperArmBone = TEXT("upperarm_l");

	/** Control Rig "Reach" variables: target (component space, per axis) and clavicle aim weight. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FString ControlRigPrimaryReachPrefix = TEXT("PrimaryReach");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FString ControlRigSupportReachPrefix = TEXT("SupportReach");

	/** Bone the chest yaw correction turns (with everything above it), and its Control Rig variable (degrees). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName ChestBone = TEXT("spine_04");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FName ControlRigChestYawVariable = TEXT("ChestYawCorrection");

	/** Control Rig variables for the chest correction axis (component space): Prefix + X/Y/Z. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FString ControlRigChestAxisPrefix = TEXT("ChestCorrectionAxis");

	/** Control Rig variables for the chest offset (component space, cm): Prefix + X/Y/Z. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|Body")
	FString ControlRigChestOffsetPrefix = TEXT("ChestOffset");

	/** Standing still (on the ground, not crouched, no montage) this long captures the locked grip. DefaultGame.ini. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "First Person|Grip|Body", meta = (ClampMin = "0.0"))
	float GripCaptureSettleSeconds = 0.3f;

	/** Neither the view nor the animated chest may turn faster than this (deg/s) while settling. DefaultGame.ini. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "First Person|Grip|Body", meta = (ClampMin = "0.0"))
	float GripCaptureMaxTurnRate = 45.0f;

	/** ABP variables feeding the TwoBoneIK nodes (component space). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|ABP")
	FName GripEnabledVariable = TEXT("bWeaponEquipped");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|ABP")
	FName RightHandEffectorVariable = TEXT("EffectorLocation_R");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|ABP")
	FName LeftHandEffectorVariable = TEXT("EffectorLocation_L");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|ABP")
	FName RightElbowTargetVariable = TEXT("JointTarget_R");

	UPROPERTY(EditDefaultsOnly, Category = "First Person|Grip|ABP")
	FName LeftElbowTargetVariable = TEXT("JointTarget_L");

	UPROPERTY(Transient)
	TObjectPtr<const UFpsWeaponGripProfile> GripProfile;

	/** Clavicle reach for the Control Rig, per arm: hand target in component space and aim weight. */
	struct FArmReach
	{
		FVector Target = FVector::ZeroVector;
		float Alpha = 0.0f;
	};
	FArmReach PrimaryReach;
	FArmReach SupportReach;

	/** Hip (0) to sights (1), see FpsAim; the weapon bone blends between the profile's placements. */
	FFpsAimBlend AimBlend;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	float AimAlpha = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	bool bAiming = false;

	FTransform ComputeWeaponSway(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, const FTransform& PinnedWeapon, const FTransform& WeaponInPrimaryHand, bool bEnabled, float DeltaSeconds);

	/** Weapon sway (pinned weapon space) and its reference, see ComputeWeaponSway. */
	FTransform SwayReference = FTransform::Identity;
	FTransform WeaponSway = FTransform::Identity;
	int32 SwayReferenceVersion = INDEX_NONE;
	FRotator LastSwayViewRotation = FRotator::ZeroRotator;
	float SwayViewYawRate = 0.0f;
	float SwayViewPitchRate = 0.0f;

	/** Low-passed shoulder position in view space, per arm (see ComputeArmReach). */
	struct FShoulderFilter
	{
		FVector Position = FVector::ZeroVector;
		bool bValid = false;
	};
	FShoulderFilter PrimaryShoulderFilter;
	FShoulderFilter SupportShoulderFilter;
	/** Low-passed view-space move of the primary shoulder that puts the elbow on its pole (UFpsFirstPersonSettings::PrimaryShoulderFitsElbow). */
	FVector PrimaryShoulderFit = FVector::ZeroVector;
	/** Low-passed elbow poles in weapon space (see FilterElbowPole). */
	FShoulderFilter PrimaryPoleFilter;
	FShoulderFilter SupportPoleFilter;

	FVector FilterElbowPole(const FVector& PoleWorld, const FTransform& WeaponWorld, FShoulderFilter& Filter, float DeltaSeconds) const;

	/** Animated shoulder in view space after the chest correction (rotation, and offset once applied). */
	FVector ShoulderInView(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, FName UpperArmBone) const;
	/** Where the shoulder should be in view space before the weapon sway: low-passed, held during fast turns. */
	FVector FilterShoulder(const FVector& TargetInView, FShoulderFilter& Filter, float DeltaSeconds) const;
	FArmReach ComputeArmReach(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, FName ClavicleBone, FName UpperArmBone, FName LowerArmBone, FName HandBone, const FVector& HandTargetWorld, const FVector& DesiredShoulderInView) const;

	/** Locked grip (see GetLockedGrip): weapon bone in primary hand space, support hand in weapon bone space. */
	FTransform LockedWeaponInPrimaryHand = FTransform::Identity;
	FTransform LockedSupportHandInWeapon = FTransform::Identity;
	/** Shoulder line in view space at grip capture, and this frame's chest correction (see UpdateChestCorrection). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	FVector LockedShoulderLineInView = FVector::LeftVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	FVector LockedShoulderLineInHeading = FVector::LeftVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	float ChestTurnWeight = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	float ChestCorrectionAngle = 0.0f;
	float LastCaptureViewYaw = 0.0f;
	float CaptureRateSeconds = 0.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	float CaptureViewTurnRate = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	float CaptureChestTurnRate = 0.0f;
	FVector LastCaptureShoulderLine = FVector::ZeroVector;
	FVector ChestCorrectionAxis = FVector::UpVector;
	FTransform ChestCorrectionWorld = FTransform::Identity;
	/** Chest offset this frame (component space, cm), see UpdateWeaponGrip. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	FVector ChestOffset = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	bool bGripLocked = false;
	int32 LockedGripVersion = 0;
	int32 AttachedGripVersion = INDEX_NONE;
	float GripSettledSeconds = 0.0f;

	/** Finger bones and their local transforms at grip capture, applied after the graph (FFpsFirstPersonAnimInstanceProxy). */
	TArray<FName> LockedFingerBones;
	TArray<FTransform> LockedFingerTransforms;
	int32 FingerLockVersion = INDEX_NONE;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "First Person|Debug")
	float FingerLockAlpha = 0.0f;

	friend struct FFpsFirstPersonAnimInstanceProxy;

	TWeakObjectPtr<APawn> CachedPawn;
	FVector LastPawnLocation = FVector::ZeroVector;
	bool bHasLastPawnLocation = false;
	TWeakObjectPtr<const UObject> HiddenBonesAppliedForMesh;
};
