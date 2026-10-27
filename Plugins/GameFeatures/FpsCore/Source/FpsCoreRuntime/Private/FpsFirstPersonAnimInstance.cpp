// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsFirstPersonAnimInstance.h"

#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_ControlRig.h"
#include "ControlRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAsset.h"
#include "Camera/PlayerCameraManager.h"
#include "FpsFirstPersonSettings.h"
#include "FpsWeaponGripProfile.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsFirstPersonAnimInstance)

/** Runs the anim graph, then puts the fingers back into the captured grip (UFpsFirstPersonAnimInstance::UpdateFingerLock). */
struct FFpsFirstPersonAnimInstanceProxy : public FAnimInstanceProxy
{
	explicit FFpsFirstPersonAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override
	{
		FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
		const UFpsFirstPersonAnimInstance* Instance = CastChecked<UFpsFirstPersonAnimInstance>(InAnimInstance);
		FingerAlpha = Instance->FingerLockAlpha;
		if (FingerVersion != Instance->FingerLockVersion)
		{
			FingerVersion = Instance->FingerLockVersion;
			FingerBones = Instance->LockedFingerBones;
			FingerTransforms = Instance->LockedFingerTransforms;
		}
	}

	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override
	{
		if (InRootNode != GetRootNode() || FingerAlpha <= 0.0f || FingerBones.IsEmpty())
		{
			return false;
		}

		EvaluateAnimationNode_WithRoot(Output, InRootNode);
		const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();
		for (int32 Index = 0; Index < FingerBones.Num(); ++Index)
		{
			const int32 MeshIndex = BoneContainer.GetPoseBoneIndexForBoneName(FingerBones[Index]);
			const FCompactPoseBoneIndex CompactIndex = MeshIndex == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
			if (CompactIndex.IsValid())
			{
				FTransform& Bone = Output.Pose[CompactIndex];
				Bone.Blend(Bone, FingerTransforms[Index], FingerAlpha);
			}
		}
		return true;
	}

	TArray<FName> FingerBones;
	TArray<FTransform> FingerTransforms;
	int32 FingerVersion = INDEX_NONE;
	float FingerAlpha = 0.0f;
};

FAnimInstanceProxy* UFpsFirstPersonAnimInstance::CreateAnimInstanceProxy()
{
	return new FFpsFirstPersonAnimInstanceProxy(this);
}

void UFpsFirstPersonAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FFpsFirstPersonAnimInstanceProxy*>(InProxy);
}

void UFpsFirstPersonAnimInstance::UpdateFingerLock(const USkeletalMeshComponent* SourceMesh, float DeltaSeconds)
{
	// The fingers keep the pose they had when the grip was captured: the locomotion animations open
	// and close them (the thumb most) with every step. Montages (reload, equip) still move them.
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	const USkinnedAsset* SkinnedAsset = SourceMesh->GetSkinnedAsset();
	if (bGripLocked && FingerLockVersion != LockedGripVersion && SkinnedAsset)
	{
		FingerLockVersion = LockedGripVersion;
		LockedFingerBones.Reset();
		LockedFingerTransforms.Reset();
		const FReferenceSkeleton& RefSkeleton = SkinnedAsset->GetRefSkeleton();
		// Captured once per grip lock, on the game thread (the source mesh has finished evaluating).
		const TArrayView<const FTransform> LocalPose = const_cast<USkeletalMeshComponent*>(SourceMesh)->GetBoneSpaceTransformsView();
		TArray<int32> Roots;
		for (const FName Root : Settings->LockedFingerRoots)
		{
			const int32 RootIndex = RefSkeleton.FindBoneIndex(Root);
			if (RootIndex != INDEX_NONE)
			{
				Roots.Add(RootIndex);
			}
		}
		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum() && BoneIndex < LocalPose.Num(); ++BoneIndex)
		{
			for (int32 Parent = BoneIndex; Parent != INDEX_NONE; Parent = RefSkeleton.GetParentIndex(Parent))
			{
				if (Roots.Contains(Parent))
				{
					LockedFingerBones.Add(RefSkeleton.GetBoneName(BoneIndex));
					LockedFingerTransforms.Add(LocalPose[BoneIndex]);
					break;
				}
			}
		}
	}

	const UAnimInstance* SourceAnimInstance = SourceMesh->GetAnimInstance();
	const bool bLock = bGripLocked && FingerLockVersion == LockedGripVersion && !LockedFingerBones.IsEmpty()
		&& !(SourceAnimInstance && SourceAnimInstance->IsAnyMontagePlaying());
	const float Target = bLock ? 1.0f : 0.0f;
	FingerLockAlpha = Settings->FingerLockBlendSpeed > 0.0f ? FMath::FInterpConstantTo(FingerLockAlpha, Target, DeltaSeconds, Settings->FingerLockBlendSpeed) : Target;
}

APawn* UFpsFirstPersonAnimInstance::GetFirstPersonPawn() const
{
	if (APawn* Pawn = CachedPawn.Get())
	{
		return Pawn;
	}

	// Cosmetic part actor -> Owner (set by UFpsAnimLayerComponent) or attach parent -> pawn.
	for (AActor* Actor = GetOwningActor(); Actor; Actor = Actor->GetOwner() ? Actor->GetOwner() : Actor->GetAttachParentActor())
	{
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			const_cast<ThisClass*>(this)->CachedPawn = Pawn;
			return Pawn;
		}
	}

	return nullptr;
}

void UFpsFirstPersonAnimInstance::SetWeaponGrip(const UFpsWeaponGripProfile* InProfile)
{
	if (GripProfile != InProfile)
	{
		// New weapon: capture its grip again.
		bGripLocked = false;
		GripSettledSeconds = 0.0f;
		++LockedGripVersion;
	}

	GripProfile = InProfile;
	if (!GripProfile)
	{
		SetBoolVariable(GripEnabledVariable, false);
	}
}

bool UFpsFirstPersonAnimInstance::GetLockedGrip(FTransform& OutWeaponInPrimaryHand, int32& OutVersion) const
{
	if (!bGripLocked)
	{
		return false;
	}

	OutWeaponInPrimaryHand = LockedWeaponInPrimaryHand;
	OutVersion = LockedGripVersion;
	return true;
}

void UFpsFirstPersonAnimInstance::UpdateGripCapture(const APawn* Pawn, const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, float DeltaSeconds)
{
	// The chest reference must come from a body that has caught up with the view: not while turning.
	const FVector ShoulderLine = ShoulderLineInView(SourceMesh, ViewToWorld);
	const float ViewYaw = ViewToWorld.Rotator().Yaw;
	// Rates over at least 50ms: animation updates can come with a near-zero delta (several per frame).
	CaptureRateSeconds += DeltaSeconds;
	if (CaptureRateSeconds >= 0.05f)
	{
		CaptureViewTurnRate = FMath::Abs(FRotator::NormalizeAxis(ViewYaw - LastCaptureViewYaw)) / CaptureRateSeconds;
		CaptureChestTurnRate = LastCaptureShoulderLine.IsNearlyZero()
			? 0.0f
			: FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ShoulderLine | LastCaptureShoulderLine, -1.0f, 1.0f))) / CaptureRateSeconds;
		LastCaptureViewYaw = ViewYaw;
		LastCaptureShoulderLine = ShoulderLine;
		CaptureRateSeconds = 0.0f;
	}
	const float ViewTurnRate = CaptureViewTurnRate;
	const float ChestTurnRate = CaptureChestTurnRate;

	if (bGripLocked)
	{
		return;
	}

	// Capture from a neutral hold: standing still on the ground, not turning, with no equip/fire montage.
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const UAnimInstance* SourceAnimInstance = SourceMesh->GetAnimInstance();
	const bool bSettled = Character && Movement && SourceAnimInstance
		&& !Character->IsCrouched()
		&& !Movement->IsFalling()
		&& Character->GetVelocity().SizeSquared() < FMath::Square(5.0f)
		&& !SourceAnimInstance->IsAnyMontagePlaying()
		&& ViewTurnRate < GripCaptureMaxTurnRate
		&& ChestTurnRate < GripCaptureMaxTurnRate;

	GripSettledSeconds = bSettled ? GripSettledSeconds + DeltaSeconds : 0.0f;
	if (GripSettledSeconds < GripCaptureSettleSeconds)
	{
		return;
	}

	const FTransform AnimWeapon = SourceMesh->GetSocketTransform(WeaponBone, RTS_World);
	LockedWeaponInPrimaryHand = AnimWeapon.GetRelativeTransform(SourceMesh->GetSocketTransform(PrimaryHandBone, RTS_World));
	LockedSupportHandInWeapon = SourceMesh->GetSocketTransform(SupportHandBone, RTS_World).GetRelativeTransform(AnimWeapon);
	LockedShoulderLineInView = ShoulderLine;
	LockedShoulderLineInHeading = ShoulderLineInHeading(SourceMesh, ViewToWorld);
	bGripLocked = true;
	++LockedGripVersion;
	OnGripLocked.Broadcast();
}

void UFpsFirstPersonAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedPawn.Reset();
	bHasLastPawnLocation = false;
	bGripLocked = false;
	GripSettledSeconds = 0.0f;
	AttachedGripVersion = INDEX_NONE;
	++LockedGripVersion;
	HiddenBonesAppliedForMesh.Reset();
	ApplyHiddenBones();
}

void UFpsFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Cosmetic parts can swap the mesh after init, which clears hidden bones.
	ApplyHiddenBones();

	if (const APawn* Pawn = GetFirstPersonPawn())
	{
		// Local controller rotation for the owner (the only viewer of this mesh).
		AimPitch = FMath::Clamp(FRotator::NormalizeAxis(Pawn->GetBaseAimRotation().Pitch), -90.0f, 90.0f);
		UpdateWeaponGrip(Pawn, DeltaSeconds);
	}

	PushControlRigVariables();
}

const USkeletalMeshComponent* UFpsFirstPersonAnimInstance::FindGripSourceMesh(const APawn* Pawn) const
{
	// The pawn's own mesh runs the full (third-person) animation, including the weapon grip pose.
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const USkeletalMeshComponent* SourceMesh = Character ? Character->GetMesh() : nullptr;
	return (SourceMesh && SourceMesh != GetSkelMeshComponent()) ? SourceMesh : nullptr;
}

void UFpsFirstPersonAnimInstance::UpdateWeaponGrip(const APawn* Pawn, float DeltaSeconds)
{
	ChestCorrectionAngle = 0.0f;
	PrimaryReach = FArmReach();
	SupportReach = FArmReach();

	const APlayerController* PlayerController = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
	const APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager.Get() : nullptr;
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	const USkeletalMeshComponent* SourceMesh = FindGripSourceMesh(Pawn);
	if (!GripProfile || !CameraManager || !MeshComponent || !SourceMesh)
	{
		SetBoolVariable(GripEnabledVariable, false);
		return;
	}

	SetBoolVariable(GripEnabledVariable, true);

	// The camera manager still holds last frame's view while the pawn has already moved this frame
	// (the mesh evaluates after movement, before the camera update). Use the current control rotation
	// and the view location the camera is about to use: true first person computes it from this
	// frame's head; legacy shifts the cached camera by the pawn's movement since that view, so the
	// weapon does not trail by one frame of movement (it was ~10cm at 60 fps while running).
	const FRotator ViewRotation = Pawn->GetViewRotation();
	const FVector PawnLocation = Pawn->GetActorLocation();
	FVector ViewLocation = CameraManager->GetCameraLocation();
	if (bHasLastPawnLocation)
	{
		ViewLocation += PawnLocation - LastPawnLocation;
	}
	LastPawnLocation = PawnLocation;
	bHasLastPawnLocation = true;
	UFpsFirstPersonSettings::GetTrueFirstPersonEyeLocation(Pawn, ViewRotation, ViewLocation);
	const FTransform ViewToWorld(ViewRotation, ViewLocation);

	UpdateGripCapture(Pawn, SourceMesh, ViewToWorld, DeltaSeconds);
	UpdateFingerLock(SourceMesh, DeltaSeconds);
	// The locked relation is only valid once the weapon visual hangs off the hand with it.
	const bool bUseLockedGrip = bGripLocked && AttachedGripVersion == LockedGripVersion;
	UpdateChestCorrection(SourceMesh, ViewToWorld, DeltaSeconds);
	const FTransform& ComponentToWorld = MeshComponent->GetComponentTransform();

	// The weapon bone is placed view-relative; the primary hand holds it with the locked relation, or
	// until that is captured, wherever the animation holds it this frame.
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	const bool bTrueFirstPerson = Settings->bTrueFirstPerson;
	const FTransform AnimWeaponToHand = bUseLockedGrip
		? LockedWeaponInPrimaryHand
		: SourceMesh->GetSocketTransform(WeaponBone, RTS_World).GetRelativeTransform(SourceMesh->GetSocketTransform(PrimaryHandBone, RTS_World));

	const FVector WeaponViewOffset = GripProfile->GetWeaponViewOffset(bTrueFirstPerson);
	const FTransform PinnedWeapon = FTransform(GripProfile->GetWeaponViewRotation(bTrueFirstPerson), WeaponViewOffset) * ViewToWorld;
	const FTransform WeaponWorld = ComputeWeaponSway(SourceMesh, ViewToWorld, PinnedWeapon, AnimWeaponToHand, bTrueFirstPerson && bUseLockedGrip, DeltaSeconds) * PinnedWeapon;
	// Points laid out for the pinned weapon, carried along with the sway.
	auto ToSwayed = [&PinnedWeapon, &WeaponWorld](const FVector& Point) { return WeaponWorld.TransformPosition(PinnedWeapon.InverseTransformPosition(Point)); };
	const FTransform PrimaryHandWorld = AnimWeaponToHand.Inverse() * WeaponWorld;
	SetVectorVariable(RightHandEffectorVariable, ComponentToWorld.InverseTransformPosition(PrimaryHandWorld.GetLocation()));
	RightHandRotation = ComponentToWorld.InverseTransformRotation(PrimaryHandWorld.GetRotation()).Rotator();

	// The support hand keeps the grip animation's relation to the primary hand (so it stays on
	// the weapon exactly as in third person).
	const FTransform AnimPrimaryHand = SourceMesh->GetSocketTransform(PrimaryHandBone, RTS_World);
	auto FromAnimation = [&AnimPrimaryHand, &PrimaryHandWorld, SourceMesh](FName Bone)
	{
		return SourceMesh->GetSocketTransform(Bone, RTS_World).GetRelativeTransform(AnimPrimaryHand) * PrimaryHandWorld;
	};

	// Elbows: blend the animated elbow with the pole where the forearm would continue straight out
	// of the hand. Straight turns the forearm to line up with the hand instead of bending the wrist,
	// but lifts the elbow toward the camera, so the blend is tunable.
	// Moving, the shoulder fit below lets the elbow reach a straighter pole; standing, a straighter pole
	// only lifted the elbow (see PrimaryWristStraightenMoving).
	const float MoveAlpha = Settings->WeaponSwayFullSpeed > 0.0f ? FMath::Clamp(Pawn->GetVelocity().Size2D() / Settings->WeaponSwayFullSpeed, 0.0f, 1.0f) : 1.0f;
	const float PrimaryStraighten = FMath::Lerp(Settings->PrimaryWristStraighten, Settings->PrimaryWristStraightenMoving, bGripLocked && Settings->PrimaryShoulderFitsElbow > 0.0f ? MoveAlpha : 0.0f);
	const FVector PrimaryPole = FilterElbowPole(FMath::Lerp(FromAnimation(PrimaryLowerArmBone).GetLocation(), StraightWristElbowTarget(PrimaryHandWorld, PrimaryHandBone), PrimaryStraighten), WeaponWorld, PrimaryPoleFilter, DeltaSeconds);
	SetVectorVariable(RightElbowTargetVariable, ComponentToWorld.InverseTransformPosition(PrimaryPole));

	SupportReach = FArmReach();
	FTransform SupportHandWorld = FTransform::Identity;
	if (GripProfile->bSupportHandFollowsAnimation)
	{
		SupportHandWorld = bUseLockedGrip ? LockedSupportHandInWeapon * WeaponWorld : FromAnimation(SupportHandBone);
		if (!FMath::IsNearlyZero(GripProfile->SupportGripRollDegrees))
		{
			// Roll the animated grip around the barrel axis (view X; the barrel is level with view X).
			const FVector PivotWorld = ToSwayed(ViewToWorld.TransformPosition(GripProfile->SupportGripPivotViewOffset + (WeaponViewOffset - GripProfile->WeaponViewOffset)));
			const FVector BarrelAxis = WeaponWorld.TransformVectorNoScale(PinnedWeapon.InverseTransformVectorNoScale(ViewToWorld.GetUnitAxis(EAxis::X)));
			const FQuat Roll(BarrelAxis, FMath::DegreesToRadians(GripProfile->SupportGripRollDegrees));
			SupportHandWorld.SetLocation(PivotWorld + Roll.RotateVector(SupportHandWorld.GetLocation() - PivotWorld));
			SupportHandWorld.SetRotation(Roll * SupportHandWorld.GetRotation());
		}
		SetVectorVariable(LeftHandEffectorVariable, ComponentToWorld.InverseTransformPosition(SupportHandWorld.GetLocation()));
		LeftHandRotation = ComponentToWorld.InverseTransformRotation(SupportHandWorld.GetRotation()).Rotator();
		const FVector SupportPole = FilterElbowPole(FMath::Lerp(FromAnimation(SupportLowerArmBone).GetLocation(), StraightWristElbowTarget(SupportHandWorld, SupportHandBone), Settings->SupportWristStraighten), WeaponWorld, SupportPoleFilter, DeltaSeconds);
		SetVectorVariable(LeftElbowTargetVariable, ComponentToWorld.InverseTransformPosition(SupportPole));
	}

	// The reach is measured on the pawn's animated pose, which is this mesh's pose only in true
	// first person (the legacy rig moves the shoulders itself).
	ChestOffset = FVector::ZeroVector;
	if (!bTrueFirstPerson)
	{
		return;
	}

	// Shoulders: low-passed in view space, then carried with the weapon sway, so the arms keep their
	// shape and move with the weapon as one piece (the hands followed the sway while the shoulders
	// stayed under a steady view, and the elbows bobbed up and down between them).
	auto SwayInView = [&ViewToWorld, &ToSwayed](const FVector& PointInView) { return ViewToWorld.InverseTransformPosition(ToSwayed(ViewToWorld.TransformPosition(PointInView))); };
	const bool bSupportArm = GripProfile->bSupportHandFollowsAnimation;
	FVector PrimaryShoulder = SwayInView(FilterShoulder(ShoulderInView(SourceMesh, ViewToWorld, PrimaryUpperArmBone), PrimaryShoulderFilter, DeltaSeconds));
	// The chest takes only part of the fit below (PrimaryShoulderFitChestShare): all of it brought the
	// neck and collar into the camera, none left the clavicle short of the fitted shoulder.
	const FVector PrimaryShoulderUnfitted = PrimaryShoulder;
	// The two-bone IK puts the elbow on its pole only when the shoulder sits one upper arm away from
	// it; otherwise the elbow slides along the pole's plane and the wrist bends. Walking changes the
	// animated arm around the hand (the pole follows), so the shoulder is moved along the line to the
	// pole by the difference (see PrimaryShoulderFitsElbow). Only while moving or turning fast: standing
	// the animated arm already matches, and the fit brought the collar toward the camera looking down.
	// Low-passed like the shoulders, or the elbow shook with it.
	FVector FitTarget = FVector::ZeroVector;
	const USkinnedAsset* SourceAsset = SourceMesh->GetSkinnedAsset();
	const int32 PrimaryLowerArmIndex = SourceAsset ? SourceAsset->GetRefSkeleton().FindBoneIndex(PrimaryLowerArmBone) : INDEX_NONE;
	if (bGripLocked && Settings->PrimaryShoulderFitsElbow > 0.0f && PrimaryLowerArmIndex != INDEX_NONE)
	{
		const float UpperArmLength = SourceAsset->GetRefSkeleton().GetRefBonePose()[PrimaryLowerArmIndex].GetTranslation().Size();
		const FVector PoleInView = ViewToWorld.InverseTransformPosition(PrimaryPole);
		const FVector Fitted = PoleInView + (PrimaryShoulder - PoleInView).GetSafeNormal() * UpperArmLength;
		FitTarget = (Fitted - PrimaryShoulder) * Settings->PrimaryShoulderFitsElbow * FMath::Max(MoveAlpha, ChestTurnWeight);
	}
	PrimaryShoulderFit = Settings->PrimaryShoulderFitSmoothingSpeed > 0.0f ? FMath::VInterpTo(PrimaryShoulderFit, FitTarget, DeltaSeconds, Settings->PrimaryShoulderFitSmoothingSpeed) : FitTarget;
	PrimaryShoulder += PrimaryShoulderFit;
	const FVector PrimaryShoulderForChest = FMath::Lerp(PrimaryShoulderUnfitted, PrimaryShoulder, Settings->PrimaryShoulderFitChestShare);
	const FVector SupportShoulder = bSupportArm ? SwayInView(FilterShoulder(ShoulderInView(SourceMesh, ViewToWorld, SupportUpperArmBone), SupportShoulderFilter, DeltaSeconds)) : FVector::ZeroVector;

	// The clavicle aim alone cannot put a shoulder anywhere (the chest under it bobs and leans with
	// the animation), so the chest is moved first to bring the shoulders to where they should be;
	// the clavicles then take the rest per arm.
	if (bGripLocked && Settings->MaxChestOffset > 0.0f)
	{
		FVector OffsetInView = PrimaryShoulderForChest - ShoulderInView(SourceMesh, ViewToWorld, PrimaryUpperArmBone);
		if (bSupportArm)
		{
			OffsetInView = 0.5f * (OffsetInView + SupportShoulder - ShoulderInView(SourceMesh, ViewToWorld, SupportUpperArmBone));
		}
		OffsetInView = OffsetInView.GetClampedToMaxSize(Settings->MaxChestOffset);

		// Never toward the camera past MinChestCameraDistance (looking down while walking, the shoulder
		// fit pulled the collar into the view): shorten the offset until the closest bone is far enough.
		if (Settings->MinChestCameraDistance > 0.0f && !Settings->ChestCameraClearanceBones.IsEmpty())
		{
			auto ClosestToCamera = [&](float Alpha)
			{
				float Closest = UE_BIG_NUMBER;
				for (const FName& Bone : Settings->ChestCameraClearanceBones)
				{
					if (SourceMesh->GetBoneIndex(Bone) != INDEX_NONE)
					{
						const FVector InView = ViewToWorld.InverseTransformPosition(ChestCorrectionWorld.TransformPosition(SourceMesh->GetSocketLocation(Bone)));
						Closest = FMath::Min(Closest, (InView + OffsetInView * Alpha).Size());
					}
				}
				return Closest;
			};
			const float Required = FMath::Min(Settings->MinChestCameraDistance, ClosestToCamera(0.0f));
			if (ClosestToCamera(1.0f) < Required)
			{
				float Low = 0.0f;
				float High = 1.0f;
				for (int32 Step = 0; Step < 8; ++Step)
				{
					const float Mid = 0.5f * (Low + High);
					(ClosestToCamera(Mid) < Required ? High : Low) = Mid;
				}
				OffsetInView *= Low;
			}
		}

		const FVector OffsetWorld = ViewToWorld.TransformVectorNoScale(OffsetInView);
		ChestCorrectionWorld = ChestCorrectionWorld * FTransform(OffsetWorld);
		ChestOffset = ComponentToWorld.InverseTransformVector(OffsetWorld);
	}

	PrimaryReach = ComputeArmReach(SourceMesh, ViewToWorld, PrimaryClavicleBone, PrimaryUpperArmBone, PrimaryLowerArmBone, PrimaryHandBone, PrimaryHandWorld.GetLocation(), PrimaryShoulder);
	PrimaryReach.Target = ComponentToWorld.InverseTransformPosition(PrimaryReach.Target);
	if (bSupportArm)
	{
		SupportReach = ComputeArmReach(SourceMesh, ViewToWorld, SupportClavicleBone, SupportUpperArmBone, SupportLowerArmBone, SupportHandBone, SupportHandWorld.GetLocation(), SupportShoulder);
		SupportReach.Target = ComponentToWorld.InverseTransformPosition(SupportReach.Target);
	}
}

FVector UFpsFirstPersonAnimInstance::ShoulderLineInHeading(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld) const
{
	// Shoulder line (right to left) in the view's heading frame (view yaw only), so it does not change
	// with view pitch.
	const FRotator Heading(0.0f, ViewToWorld.Rotator().Yaw, 0.0f);
	return Heading.UnrotateVector(SourceMesh->GetSocketLocation(SupportUpperArmBone) - SourceMesh->GetSocketLocation(PrimaryUpperArmBone)).GetSafeNormal();
}

FVector UFpsFirstPersonAnimInstance::ShoulderLineInView(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld) const
{
	// Shoulder line (right to left) in view space.
	return ViewToWorld.InverseTransformVectorNoScale(SourceMesh->GetSocketLocation(SupportUpperArmBone) - SourceMesh->GetSocketLocation(PrimaryUpperArmBone)).GetSafeNormal();
}

void UFpsFirstPersonAnimInstance::UpdateChestCorrection(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, float DeltaSeconds)
{
	// Fast turns leave the animated upper body (aim offset / turn in place) facing behind the view and
	// leaning into the turn, which swings the support shoulder forward and lifts its elbow into the
	// lower left of the screen. Turn the 1P chest so its shoulder line is back where it was, in the
	// view's heading frame, when the grip was captured.
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	ChestCorrectionAngle = 0.0f;
	ChestCorrectionAxis = FVector::UpVector;
	ChestCorrectionWorld = FTransform::Identity;
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (!Settings->bTrueFirstPerson || !bGripLocked || Settings->MaxChestYawCorrection <= 0.0f || !MeshComponent || SourceMesh->GetBoneIndex(ChestBone) == INDEX_NONE)
	{
		return;
	}

	// Only while turning: the lag (yaw, and the lean into the turn) builds up when the view turns
	// faster than the body can follow. Standing, walking or looking down the animation is left
	// alone (correcting it there made walking and looking down worse). The weight rises with the
	// view turn rate and decays once the turn stops, as the body catches up.
	const float TurnTarget = FMath::GetRangePct(Settings->ChestTurnRateStart, FMath::Max(Settings->ChestTurnRateFull, Settings->ChestTurnRateStart + 1.0f), CaptureViewTurnRate);
	const float Target = FMath::Clamp(TurnTarget, 0.0f, 1.0f);
	ChestTurnWeight = Target >= ChestTurnWeight ? Target : FMath::FInterpTo(ChestTurnWeight, Target, DeltaSeconds, Settings->ChestCorrectionReleaseSpeed);
	if (ChestTurnWeight <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FRotator Heading(0.0f, ViewToWorld.Rotator().Yaw, 0.0f);
	const FVector Current = Heading.RotateVector(ShoulderLineInHeading(SourceMesh, ViewToWorld));
	const FVector Locked = Heading.RotateVector(LockedShoulderLineInHeading);
	FVector AxisWorld;
	float Angle;
	FQuat::FindBetweenNormals(Current, Locked).ToAxisAndAngle(AxisWorld, Angle);
	Angle = FMath::Min(Angle * ChestTurnWeight, FMath::DegreesToRadians(Settings->MaxChestYawCorrection));
	const FQuat Turn(AxisWorld, Angle);

	ChestCorrectionAngle = FMath::RadiansToDegrees(Angle);
	ChestCorrectionAxis = MeshComponent->GetComponentTransform().InverseTransformVectorNoScale(AxisWorld).GetSafeNormal();
	const FVector Pivot = SourceMesh->GetSocketLocation(ChestBone);
	ChestCorrectionWorld = FTransform(-Pivot) * FTransform(Turn) * FTransform(Pivot);
}

FTransform UFpsFirstPersonAnimInstance::ComputeWeaponSway(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, const FTransform& PinnedWeapon, const FTransform& WeaponInPrimaryHand, bool bEnabled, float DeltaSeconds)
{
	// The weapon follows how the animated body moves it (locomotion bob, short lags) within limits,
	// instead of staying locked to the view: a locked weapon made the arms absorb every body motion
	// (elbows jerking, bent wrists while walking).
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	if (!bEnabled || Settings->WeaponFollowsBody <= 0.0f)
	{
		SwayReferenceVersion = INDEX_NONE;
		WeaponSway = FTransform::Identity;
		return WeaponSway;
	}

	const FTransform BodyHand = SourceMesh->GetSocketTransform(PrimaryHandBone, RTS_World) * ChestCorrectionWorld;
	const FTransform BodyWeaponInPinned = (WeaponInPrimaryHand * BodyHand).GetRelativeTransform(PinnedWeapon);
	// The sway is the body carry minus its slow average (a high-pass): the bob and short lags pass,
	// a lasting offset does not, so the framing stays the profile's. (A one-time reference taken at
	// grip capture left the idle gun 3.5cm low and popped it back up whenever the sway faded out.)
	if (SwayReferenceVersion != LockedGripVersion)
	{
		SwayReference = BodyWeaponInPinned;
		SwayReferenceVersion = LockedGripVersion;
		WeaponSway = FTransform::Identity;
	}
	else if (Settings->WeaponSwayCenterSpeed > 0.0f)
	{
		SwayReference.SetTranslation(FMath::VInterpTo(SwayReference.GetTranslation(), BodyWeaponInPinned.GetTranslation(), DeltaSeconds, Settings->WeaponSwayCenterSpeed));
		SwayReference.SetRotation(FMath::QInterpTo(SwayReference.GetRotation(), BodyWeaponInPinned.GetRotation(), DeltaSeconds, Settings->WeaponSwayCenterSpeed));
	}

	// While the view turns fast the body lags behind it (see UpdateChestCorrection); following it then
	// swung the whole viewmodel 9cm the wrong way, so the body follow fades out with the turn and the
	// weapon only lags behind the turn (inertia) instead.
	// Standing still the weapon stays where the profile frames it (posture changes in place moved it).
	const APawn* Pawn = GetFirstPersonPawn();
	const float SpeedFollow = Settings->WeaponSwayFullSpeed > 0.0f && Pawn ? FMath::Clamp(Pawn->GetVelocity().Size2D() / Settings->WeaponSwayFullSpeed, 0.0f, 1.0f) : 1.0f;
	const float Follow = Settings->WeaponFollowsBody * (1.0f - ChestTurnWeight) * SpeedFollow;
	const FTransform Target = SwayReference.Inverse() * BodyWeaponInPinned;
	const FVector Offset = Target.GetTranslation().GetClampedToMaxSize(Settings->WeaponSwayMaxOffset) * Follow;
	FVector Axis;
	float Angle;
	FQuat Rotation = Target.GetRotation();
	if (Rotation.W < 0.0f)
	{
		Rotation = -Rotation;
	}
	Rotation.ToAxisAndAngle(Axis, Angle);
	Angle = FMath::Min(Angle, FMath::DegreesToRadians(Settings->WeaponSwayMaxAngle)) * Follow;

	// Turn inertia: the weapon trails the view rotation a little (opposite to the turn), in view axes.
	const FRotator ViewRotation = ViewToWorld.Rotator();
	if (DeltaSeconds > 1.0e-3f)
	{
		SwayViewYawRate = FRotator::NormalizeAxis(ViewRotation.Yaw - LastSwayViewRotation.Yaw) / DeltaSeconds;
		SwayViewPitchRate = FRotator::NormalizeAxis(ViewRotation.Pitch - LastSwayViewRotation.Pitch) / DeltaSeconds;
		LastSwayViewRotation = ViewRotation;
	}
	const float LagScale = Settings->WeaponTurnLag * 0.01f;
	const FRotator LagInView(
		FMath::Clamp(-SwayViewPitchRate * LagScale, -Settings->WeaponTurnLagMax, Settings->WeaponTurnLagMax),
		FMath::Clamp(-SwayViewYawRate * LagScale, -Settings->WeaponTurnLagMax, Settings->WeaponTurnLagMax),
		0.0f);
	const FQuat ViewQuat = ViewToWorld.GetRotation();
	const FQuat PinnedQuat = PinnedWeapon.GetRotation();
	const FQuat LagInPinned = PinnedQuat.Inverse() * (ViewQuat * LagInView.Quaternion() * ViewQuat.Inverse()) * PinnedQuat;
	const FQuat TargetRotation = LagInPinned * FQuat(Axis, Angle);

	const float Speed = Settings->WeaponSwaySmoothing;
	WeaponSway.SetTranslation(Speed > 0.0f ? FMath::VInterpTo(WeaponSway.GetTranslation(), Offset, DeltaSeconds, Speed) : Offset);
	WeaponSway.SetRotation(Speed > 0.0f ? FMath::QInterpTo(WeaponSway.GetRotation(), TargetRotation, DeltaSeconds, Speed) : TargetRotation);
	return WeaponSway;
}

FVector UFpsFirstPersonAnimInstance::FilterElbowPole(const FVector& PoleWorld, const FTransform& WeaponWorld, FShoulderFilter& Filter, float DeltaSeconds) const
{
	// The animated elbow (relative to the hand) swings with the walk cycle; as the pole it turned the
	// arm's bend plane every step. Low-pass it in weapon space: the arm keeps its shape around the
	// weapon, slow changes (aim offset) still pass.
	const float Speed = GetDefault<UFpsFirstPersonSettings>()->ElbowPoleSmoothingSpeed;
	const FVector PoleInWeapon = WeaponWorld.InverseTransformPosition(PoleWorld);
	if (!Filter.bValid || Speed <= 0.0f)
	{
		Filter.Position = PoleInWeapon;
		Filter.bValid = true;
	}
	else
	{
		Filter.Position = FMath::VInterpTo(Filter.Position, PoleInWeapon, DeltaSeconds, Speed);
	}
	return WeaponWorld.TransformPosition(Filter.Position);
}

FVector UFpsFirstPersonAnimInstance::ShoulderInView(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, FName UpperArmBone) const
{
	return ViewToWorld.InverseTransformPosition(ChestCorrectionWorld.TransformPosition(SourceMesh->GetSocketLocation(UpperArmBone)));
}

FVector UFpsFirstPersonAnimInstance::FilterShoulder(const FVector& TargetInView, FShoulderFilter& Filter, float DeltaSeconds) const
{
	// Where the shoulder should be, in view space. The hands hold the weapon, so every bob of the
	// animated shoulders (walk cycle, turn) went straight into the elbows, which jerked up and down.
	// The shoulder follows the animation through a low-pass filter instead: slow changes (aim offset,
	// crouch) pass, the quick bob does not.
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	const FVector& Current = TargetInView;
	if (!Filter.bValid || Settings->ShoulderSmoothingSpeed <= 0.0f)
	{
		Filter.Position = Current;
		Filter.bValid = true;
	}
	else
	{
		// While the view turns fast the body lags and twists below the chest, pushing the shoulders
		// forward (the support upper arm then fills the lower left of the screen): the filter holds
		// them where they were relative to the view when the turn began, with the chest correction's
		// weight. (Pulling them to the position captured with the grip moved them as a turn began.)
		const float Hold = bGripLocked ? ChestTurnWeight : 0.0f;
		const float Speed = Settings->ShoulderSmoothingSpeed * (1.0f - Hold);
		if (Speed > KINDA_SMALL_NUMBER)
		{
			Filter.Position = FMath::VInterpTo(Filter.Position, Current, DeltaSeconds, Speed);
		}
	}
	return Filter.Position;
}

UFpsFirstPersonAnimInstance::FArmReach UFpsFirstPersonAnimInstance::ComputeArmReach(const USkeletalMeshComponent* SourceMesh, const FTransform& ViewToWorld, FName ClavicleBone, FName UpperArmBone, FName LowerArmBone, FName HandBone, const FVector& HandTargetWorld, const FVector& DesiredShoulderInView) const
{
	FArmReach Reach;
	Reach.Target = HandTargetWorld;

	const USkinnedAsset* SkinnedAsset = SourceMesh->GetSkinnedAsset();
	if (!SkinnedAsset)
	{
		return Reach;
	}

	const FReferenceSkeleton& RefSkeleton = SkinnedAsset->GetRefSkeleton();
	const int32 LowerArmIndex = RefSkeleton.FindBoneIndex(LowerArmBone);
	const int32 HandIndex = RefSkeleton.FindBoneIndex(HandBone);
	if (LowerArmIndex == INDEX_NONE || HandIndex == INDEX_NONE || RefSkeleton.FindBoneIndex(ClavicleBone) == INDEX_NONE)
	{
		return Reach;
	}

	const float ArmLength = RefSkeleton.GetRefBonePose()[LowerArmIndex].GetTranslation().Size() + RefSkeleton.GetRefBonePose()[HandIndex].GetTranslation().Size();
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	const FVector Shoulder = ShoulderInView(SourceMesh, ViewToWorld, UpperArmBone);
	FVector Desired = DesiredShoulderInView;

	// Out of reach: the shoulder comes forward toward the hand by the shortfall.
	const FVector HandInView = ViewToWorld.InverseTransformPosition(HandTargetWorld);
	const float Shortfall = FVector::Dist(HandInView, Desired) - ArmLength * Settings->ReachArmFraction;
	if (Shortfall > 0.0f)
	{
		Desired += (HandInView - Desired).GetSafeNormal() * Shortfall;
	}

	// Never closer to the camera than MinShoulderCameraDistance: closer, the shoulder mesh cuts the
	// near plane at the left of the screen.
	if (Desired.Size() < Settings->MinShoulderCameraDistance)
	{
		Desired = Desired.GetSafeNormal() * Settings->MinShoulderCameraDistance;
	}

	if (FVector::DistSquared(Desired, Shoulder) < FMath::Square(0.1f))
	{
		return Reach;
	}

	// The Control Rig aims the clavicle at this point; the shoulder lands on that line.
	Reach.Target = ViewToWorld.TransformPosition(Desired);
	Reach.Alpha = 1.0f;
	return Reach;
}

FVector UFpsFirstPersonAnimInstance::StraightWristElbowTarget(const FTransform& HandWorld, FName HandBone) const
{
	// In the reference pose the wrist is straight: the hand's offset from its parent (lower arm)
	// gives the forearm direction, expressed here in the hand's own space.
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	const USkinnedAsset* SkinnedAsset = MeshComponent ? MeshComponent->GetSkinnedAsset() : nullptr;
	const int32 HandIndex = SkinnedAsset ? SkinnedAsset->GetRefSkeleton().FindBoneIndex(HandBone) : INDEX_NONE;
	if (HandIndex == INDEX_NONE)
	{
		return HandWorld.GetLocation();
	}

	const FTransform& HandRefLocal = SkinnedAsset->GetRefSkeleton().GetRefBonePose()[HandIndex];
	const float ForearmLength = HandRefLocal.GetTranslation().Size();
	const FVector ForearmAxisInHand = HandRefLocal.GetRotation().UnrotateVector(HandRefLocal.GetTranslation()).GetSafeNormal();

	// Elbow position that makes elbow -> wrist continue the hand's forearm axis.
	return HandWorld.GetLocation() - HandWorld.GetRotation().RotateVector(ForearmAxisInHand) * ForearmLength;
}

UControlRig* UFpsFirstPersonAnimInstance::FindFirstPersonControlRig() const
{
	const IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(GetClass());
	if (!AnimClassInterface)
	{
		return nullptr;
	}

	for (const FStructProperty* NodeProperty : AnimClassInterface->GetAnimNodeProperties())
	{
		if (NodeProperty && NodeProperty->Struct && NodeProperty->Struct->IsChildOf(FAnimNode_ControlRig::StaticStruct()))
		{
			const FAnimNode_ControlRig* ControlRigNode = NodeProperty->ContainerPtrToValuePtr<FAnimNode_ControlRig>(this);
			return ControlRigNode ? ControlRigNode->GetControlRig() : nullptr;
		}
	}

	return nullptr;
}

void UFpsFirstPersonAnimInstance::PushControlRigVariables()
{
	if (ControlRigPitchVariable.IsNone())
	{
		return;
	}

	// Game thread, before this frame's parallel update/evaluate reads the rig.
	UControlRig* ControlRig = FindFirstPersonControlRig();
	if (!ControlRig)
	{
		return;
	}

	auto SetRigFloat = [ControlRig](FName VariableName, float Value)
	{
		FNumericProperty* Property = VariableName.IsNone() ? nullptr : CastField<FNumericProperty>(ControlRig->GetClass()->FindPropertyByName(VariableName));
		if (Property && Property->IsFloatingPoint())
		{
			Property->SetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(ControlRig), Value);
		}
	};

	SetRigFloat(ControlRigPitchVariable, AimPitch);
	// Chest correction as axis (component space) and angle in degrees.
	SetRigFloat(ControlRigChestYawVariable, GripProfile ? ChestCorrectionAngle : 0.0f);
	SetRigFloat(FName(ControlRigChestAxisPrefix + TEXT("X")), ChestCorrectionAxis.X);
	SetRigFloat(FName(ControlRigChestAxisPrefix + TEXT("Y")), ChestCorrectionAxis.Y);
	SetRigFloat(FName(ControlRigChestAxisPrefix + TEXT("Z")), ChestCorrectionAxis.Z);
	const FVector RigChestOffset = GripProfile ? ChestOffset : FVector::ZeroVector;
	SetRigFloat(FName(ControlRigChestOffsetPrefix + TEXT("X")), RigChestOffset.X);
	SetRigFloat(FName(ControlRigChestOffsetPrefix + TEXT("Y")), RigChestOffset.Y);
	SetRigFloat(FName(ControlRigChestOffsetPrefix + TEXT("Z")), RigChestOffset.Z);

	auto SetRigReach = [&SetRigFloat](const FString& Prefix, const FArmReach& Reach)
	{
		SetRigFloat(FName(Prefix + TEXT("Alpha")), Reach.Alpha);
		SetRigFloat(FName(Prefix + TEXT("X")), Reach.Target.X);
		SetRigFloat(FName(Prefix + TEXT("Y")), Reach.Target.Y);
		SetRigFloat(FName(Prefix + TEXT("Z")), Reach.Target.Z);
	};
	SetRigReach(ControlRigPrimaryReachPrefix, GripProfile ? PrimaryReach : FArmReach());
	SetRigReach(ControlRigSupportReachPrefix, GripProfile ? SupportReach : FArmReach());
	// True first person keeps the animated pose; the rig only runs for the legacy setup.
	SetRigFloat(ControlRigAlphaVariable, GetDefault<UFpsFirstPersonSettings>()->bTrueFirstPerson ? 0.0f : 1.0f);
}

void UFpsFirstPersonAnimInstance::SetBoolVariable(FName VariableName, bool bValue)
{
	if (VariableName.IsNone())
	{
		return;
	}

	if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(GetClass(), VariableName))
	{
		BoolProperty->SetPropertyValue_InContainer(this, bValue);
	}
}

void UFpsFirstPersonAnimInstance::SetVectorVariable(FName VariableName, const FVector& Value)
{
	if (VariableName.IsNone())
	{
		return;
	}

	if (FStructProperty* VectorProperty = FindFProperty<FStructProperty>(GetClass(), VariableName))
	{
		if (VectorProperty->Struct == TBaseStructure<FVector>::Get())
		{
			*VectorProperty->ContainerPtrToValuePtr<FVector>(this) = Value;
		}
	}
}

void UFpsFirstPersonAnimInstance::ApplyHiddenBones()
{
	USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (!MeshComponent || !MeshComponent->GetSkeletalMeshAsset() || HiddenBonesAppliedForMesh.Get() == MeshComponent->GetSkeletalMeshAsset())
	{
		return;
	}
	HiddenBonesAppliedForMesh = MeshComponent->GetSkeletalMeshAsset();

	// With first person rendering the legs are drawn by a world layer copy (UFpsFirstPersonWeaponComponent).
	TArray<FName> BonesToHide = HiddenBoneNames;
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	if (Settings->UseFirstPersonRendering())
	{
		BonesToHide.Append(Settings->WorldLayerBones);
	}

	for (const FName& BoneName : BonesToHide)
	{
		if (!BoneName.IsNone() && !MeshComponent->IsBoneHiddenByName(BoneName) && MeshComponent->GetBoneIndex(BoneName) != INDEX_NONE)
		{
			MeshComponent->HideBoneByName(BoneName, PBO_None);
		}
	}
}
