// Copyright Epic Games, Inc. All Rights Reserved.

// First-person rig regression test. Drives the local pawn in PIE through a list of states (look
// angle, crouch, walk, strafe, turn), samples the 1P body, the 3P body (shadow) and the 1P gun every
// frame, and fails on every symptom that was seen once (gun through the torso, feet off the shadow,
// bent wrists, hands sliding on the gun, elbows high or bobbing, ...). Limits: FpsRigLimits below.
//
// Session Frontend > Automation > FpsCore.FirstPersonRig.Quick / Full, or from a command line:
//   UnrealEditor-Cmd UE5NetworkFps.uproject -nullrhi -unattended -nosplash -nosound -UseFixedTimeStep -FPS=50
//     -ExecCmds="Automation RunTests FpsCore.FirstPersonRig.Full; Quit" -TestExit="Automation Test Queue Empty"
// Tools/FpRigRegression/run_fp_rig_test.ps1 wraps that. Quick while iterating, Full before committing.

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace FpsRig
{
	// Flat 200 m slab with one PlayerStart tagged FpRigStart: walking, strafing and turning never
	// reach a wall or a ledge (on the firing range the random spawn made results change per run).
	const TCHAR* TestMap = TEXT("/FpsMaps/Maps/L_FpsRigTest");
	const FName StartTag = TEXT("FpRigStart");

	struct FState
	{
		const TCHAR* Name;
		float Pitch;
		bool bCrouch;
		FVector2D Move; // forward, right
		float TurnRate; // yaw deg/s; per frame made it depend on the editor fps
		bool bQuick;
	};

	// "_" states are not recorded: stand still first so equip finishes and the grip locks.
	// 23 -> 11 states on 2026-10-27: over 64 recorded sweeps each dropped state either never failed
	// or only ever failed together with a kept one (table in Tools/FpRigRegression/README.md).
	// Quick = the kept states that failed alone most often, plus stand_0 (idle reference for
	// "elbows stay down while turning") and stand_dn89 (torso clearance).
	const FState States[] = {
		{TEXT("_settle"), 0, false, {0, 0}, 0, true},
		{TEXT("stand_up60"), 60, false, {0, 0}, 0, false},
		{TEXT("stand_0"), 0, false, {0, 0}, 0, true},
		{TEXT("stand_dn89"), -89, false, {0, 0}, 0, true},
		{TEXT("crouch_dn85"), -85, true, {0, 0}, 0, false},
		{TEXT("walk_back_dn60"), -60, false, {-1, 0}, 0, false},
		{TEXT("walk_fwd_dn85"), -85, false, {1, 0}, 0, true},
		{TEXT("strafe_l_0"), 0, false, {0, -1}, 0, true},
		{TEXT("strafe_r_0"), 0, false, {0, 1}, 0, false},
		{TEXT("walk_back_crouch_dn60"), -60, true, {-1, 0}, 0, true},
		// 150 / 1250 deg/s = the former 3 / 25 deg per frame at ~50 fps
		{TEXT("turn_in_place_0"), 0, false, {0, 0}, 150, false},
		// fast mouse turns: the animated upper body lags the view and lifted the left elbow
		{TEXT("turn_fast_l_0"), 0, false, {0, 0}, -1250, true},
	};

	constexpr int32 WarmupFrames = 15;
	constexpr int32 SampleFrames = 15;
	// and at least this long: crouch transitions and the arm filters (time constant ~0.33 s) must
	// settle before sampling; 0.6 s measured them still converging
	constexpr float WarmupSeconds = 1.5f;
	// settle until the 1P weapon hangs off the hand (grip captured), at most this many frames
	constexpr int32 SettleMaxFrames = 600;
	constexpr double PawnTimeoutSeconds = 120.0;
	constexpr double SweepTimeoutSeconds = 600.0;

	// Every limit comes from a symptom that was seen once; do not loosen one to make a fix pass.
	namespace Limits
	{
		constexpr float TorsoClearanceMin = 14.f;      // look down: gun through belly/hips (twice)
		constexpr float FeetStaticMax = 1.f;           // owner shadow feet vs 1P feet
		constexpr float FeetMovingMax = 3.f;
		constexpr float WristBendLMax = 30.f;          // bent support wrist (twice); absolute
		constexpr float WristBendVs3PMax = 10.f;       // either wrist vs the same frame of the 3P animation
		constexpr float HandInGunRangeMax = 1.5f;      // hands sliding on the gun (walking backwards)
		constexpr float MuzzleViewStaticRangeMax = 2.f; // gun drifting on screen while standing still
		constexpr float BarrelViewRangeDegMax = 13.f;  // barrel vs view; sway is WeaponSwayMaxAngle (6) each way
		constexpr float ElbowViewZMax = -8.f;          // elbows high, arms filling the screen
		constexpr float BodyNearCameraMin = 14.f;      // camera inside the neck/collar
		constexpr float ShoulderNearCameraMin = 20.f;  // shoulder clipped at the left screen edge
		constexpr float ElbowRiseVsIdleMax = 1.5f;     // fast turns lifted the elbow
		constexpr float ElbowBobMax = 1.5f;            // elbows jerking up/down while walking/turning
		constexpr float ElbowVsGunRangeMax = 1.f;      // elbow moving against the gun
		constexpr float FingerRangeMax = 0.3f;         // thumb flapping on the grip while walking
	}

	const FName Torso[] = {TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("thigh_l"), TEXT("thigh_r")};
	const FName GunPoly[] = {TEXT("Grip"), TEXT("Magazine"), TEXT("Barrel"), TEXT("Muzzle")};
	// visible bones closest to the eyes (head is hidden)
	const FName NearCamera[] = {TEXT("neck_01"), TEXT("neck_02"), TEXT("spine_05"), TEXT("clavicle_l"), TEXT("clavicle_r")};
	const FName Shoulders[] = {TEXT("upperarm_l"), TEXT("upperarm_r"), TEXT("upperarm_twist_01_l"), TEXT("upperarm_twist_01_r")};
	const FName Feet[] = {TEXT("foot_l"), TEXT("foot_r"), TEXT("ball_l"), TEXT("ball_r")};

	TArray<FName> FingerJoints()
	{
		TArray<FName> Out;
		for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
		{
			for (const TCHAR* J : {TEXT("thumb_02"), TEXT("thumb_03"), TEXT("index_01"), TEXT("index_02"), TEXT("index_03"),
				TEXT("middle_01"), TEXT("middle_02"), TEXT("middle_03"), TEXT("ring_01"), TEXT("ring_02"), TEXT("ring_03"),
				TEXT("pinky_01"), TEXT("pinky_02"), TEXT("pinky_03")})
			{
				Out.Add(*FString::Printf(TEXT("%s_%s"), J, Side));
			}
		}
		return Out;
	}

	struct FSample
	{
		float TorsoClearance = 0;
		FName TorsoClosestBone;
		float FeetVsShadow = 0;
		float BodyNearCamera = 0;
		float ShoulderNearCamera = 0;
		FVector MuzzleView;
		FVector BarrelViewDeg;
		FVector HandLInGun, HandRInGun;
		float WristBendL = 0, WristBendR = 0, WristBendL3P = 0, WristBendR3P = 0;
		FVector ElbowLView, ElbowRView;
		FVector ElbowLInGun, ElbowRInGun;
		TArray<FVector> Fingers1P, Fingers3P; // FingerJoints() order, in their hand's space
	};

	struct FStatic
	{
		TArray<TPair<FString, bool>> Checks;
	};

	struct FMeshes
	{
		APlayerController* PC = nullptr;
		APawn* Pawn = nullptr;
		USkeletalMeshComponent* FP = nullptr;  // FirstPersonMesh: what the owner sees
		USkeletalMeshComponent* TP = nullptr;  // MeshComponent: 3P body, the owner's shadow
		USkeletalMeshComponent* Gun1P = nullptr;
		USkeletalMeshComponent* Gun3P = nullptr;
		bool IsValid() const { return PC && Pawn && FP && TP && Gun1P; }
	};

	FMeshes Find(UWorld* World)
	{
		FMeshes M;
		M.PC = World ? World->GetFirstPlayerController() : nullptr;
		M.Pawn = M.PC ? M.PC->GetPawn() : nullptr;
		if (!M.Pawn)
		{
			return M;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<USkeletalMeshComponent*> Meshes(*It);
			if (It->GetAttachParentActor() == M.Pawn)
			{
				for (USkeletalMeshComponent* Mesh : Meshes)
				{
					if (Mesh->GetName() == TEXT("FirstPersonMesh")) { M.FP = Mesh; }
					else if (Mesh->GetName() == TEXT("MeshComponent")) { M.TP = Mesh; }
				}
			}
			if (It->GetClass()->GetName().Contains(TEXT("Rifle")))
			{
				for (USkeletalMeshComponent* Mesh : Meshes)
				{
					(Mesh->bOnlyOwnerSee ? M.Gun1P : M.Gun3P) = Mesh;
				}
			}
		}
		return M;
	}

	float SegDist(const FVector& Q, const FVector& A, const FVector& B)
	{
		return FMath::PointDistToSegment(Q, A, B);
	}

	float WristBend(USkeletalMeshComponent* Mesh, FName Lower, FName Hand)
	{
		const FVector Fore = (Mesh->GetSocketLocation(Hand) - Mesh->GetSocketLocation(Lower)).GetSafeNormal();
		FVector HandX = Mesh->GetSocketRotation(Hand).Vector();
		// Manny right-side bones point -X toward the child.
		if (Hand.ToString().EndsWith(TEXT("_r")))
		{
			HandX *= -1.0;
		}
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Fore | HandX, -1.0, 1.0)));
	}

	FTransform View(const FMeshes& M)
	{
		const APlayerCameraManager* Cam = M.PC->PlayerCameraManager;
		return FTransform(Cam->GetCameraRotation(), Cam->GetCameraLocation());
	}

	float ChestYaw(const FTransform& V, USkeletalMeshComponent* Mesh)
	{
		const FVector A = V.InverseTransformPosition(Mesh->GetSocketLocation(TEXT("upperarm_l")));
		const FVector B = V.InverseTransformPosition(Mesh->GetSocketLocation(TEXT("upperarm_r")));
		return FMath::RadiansToDegrees(FMath::Atan2(A.Y - B.Y, A.X - B.X));
	}

	TArray<FVector> Fingers(USkeletalMeshComponent* Mesh, const TArray<FName>& Joints)
	{
		TArray<FVector> Out;
		for (const FName& J : Joints)
		{
			const FString Side = J.ToString().Right(1);
			Out.Add(Mesh->GetSocketTransform(*(TEXT("hand_") + Side)).InverseTransformPosition(Mesh->GetSocketLocation(J)));
		}
		return Out;
	}

	FSample Sample(const FMeshes& M, const TArray<FName>& Joints)
	{
		FSample S;
		const FTransform V = View(M);
		const FTransform GunXf = M.Gun1P->GetSocketTransform(TEXT("Grip"));
		TArray<FVector> Poly;
		for (const FName& Socket : GunPoly) { Poly.Add(M.Gun1P->GetSocketLocation(Socket)); }
		S.TorsoClearance = TNumericLimits<float>::Max();
		for (const FName& Bone : Torso)
		{
			const FVector Q = M.FP->GetSocketLocation(Bone);
			for (int32 i = 0; i + 1 < Poly.Num(); ++i)
			{
				const float D = SegDist(Q, Poly[i], Poly[i + 1]);
				if (D < S.TorsoClearance) { S.TorsoClearance = D; S.TorsoClosestBone = Bone; }
			}
		}
		for (const FName& Bone : Feet)
		{
			S.FeetVsShadow = FMath::Max(S.FeetVsShadow, (float)FVector::Dist(M.FP->GetSocketLocation(Bone), M.TP->GetSocketLocation(Bone)));
		}
		const FVector CamLoc = M.PC->PlayerCameraManager->GetCameraLocation();
		S.BodyNearCamera = S.ShoulderNearCamera = TNumericLimits<float>::Max();
		for (const FName& Bone : NearCamera) { S.BodyNearCamera = FMath::Min(S.BodyNearCamera, (float)FVector::Dist(M.FP->GetSocketLocation(Bone), CamLoc)); }
		for (const FName& Bone : Shoulders) { S.ShoulderNearCamera = FMath::Min(S.ShoulderNearCamera, (float)FVector::Dist(M.FP->GetSocketLocation(Bone), CamLoc)); }
		S.MuzzleView = V.InverseTransformPosition(M.Gun1P->GetSocketLocation(TEXT("Muzzle")));
		const FVector D = S.MuzzleView - V.InverseTransformPosition(M.Gun1P->GetSocketLocation(TEXT("Barrel")));
		S.BarrelViewDeg = FVector(FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)), FMath::RadiansToDegrees(FMath::Atan2(D.Z, D.X)), 0);
		S.HandLInGun = GunXf.InverseTransformPosition(M.FP->GetSocketLocation(TEXT("hand_l")));
		S.HandRInGun = GunXf.InverseTransformPosition(M.FP->GetSocketLocation(TEXT("hand_r")));
		S.WristBendL = WristBend(M.FP, TEXT("lowerarm_l"), TEXT("hand_l"));
		S.WristBendR = WristBend(M.FP, TEXT("lowerarm_r"), TEXT("hand_r"));
		// same pose on the 3P (shadow) mesh: the rifle pistol grip bends the right wrist by design
		S.WristBendL3P = WristBend(M.TP, TEXT("lowerarm_l"), TEXT("hand_l"));
		S.WristBendR3P = WristBend(M.TP, TEXT("lowerarm_r"), TEXT("hand_r"));
		S.ElbowLView = V.InverseTransformPosition(M.FP->GetSocketLocation(TEXT("lowerarm_l")));
		S.ElbowRView = V.InverseTransformPosition(M.FP->GetSocketLocation(TEXT("lowerarm_r")));
		// the arm's shape around the gun: moving with the gun is sway, moving against it is the elbow bob
		S.ElbowLInGun = GunXf.InverseTransformPosition(M.FP->GetSocketLocation(TEXT("lowerarm_l")));
		S.ElbowRInGun = GunXf.InverseTransformPosition(M.FP->GetSocketLocation(TEXT("lowerarm_r")));
		S.Fingers1P = Fingers(M.FP, Joints);
		S.Fingers3P = Fingers(M.TP, Joints);
		return S;
	}

	FStatic StaticChecks(const FMeshes& M)
	{
		FStatic Out;
		auto Add = [&Out](const TCHAR* Name, bool bOk) { Out.Checks.Emplace(Name, bOk); };
		USkeletalMeshComponent* Legs = nullptr;
		int32 LegsCount = 0;
		for (USkeletalMeshComponent* Mesh : TInlineComponentArray<USkeletalMeshComponent*>(M.Pawn))
		{
			if (Mesh->GetName() == TEXT("FpsWorldLayerLegs")) { Legs = Mesh; ++LegsCount; }
		}
		const UCameraComponent* Cam = M.Pawn->FindComponentByClass<UCameraComponent>();
		const float Scale = Cam && Cam->bEnableFirstPersonScale ? Cam->FirstPersonScale : 1.f;
		const float Radius = M.Pawn->FindComponentByClass<UCapsuleComponent>()->GetScaledCapsuleRadius();
		const float Muzzle = View(M).InverseTransformPosition(M.Gun1P->GetSocketLocation(TEXT("Muzzle"))).Size();
		using EFP = EFirstPersonPrimitiveType;
		// owner shadow comes from the 3P body (engine World Space Representation)
		Add(TEXT("body3p_world_space_representation_casts_shadow"), M.TP->FirstPersonPrimitiveType == EFP::WorldSpaceRepresentation && M.TP->CastShadow);
		Add(TEXT("body1p_no_shadow"), !M.FP->CastShadow);
		Add(TEXT("gun3p_hidden_no_owner_shadow"), M.Gun3P && M.Gun3P->bOwnerNoSee);
		Add(TEXT("gun1p_no_shadow"), !M.Gun1P->CastShadow);
		Add(TEXT("head_hidden_1p"), M.FP->IsBoneHiddenByName(TEXT("head")));
		// gun through wall / own thigh while turning: arms, upper body and gun in the first person layer
		Add(TEXT("body1p_first_person_layer"), M.FP->FirstPersonPrimitiveType == EFP::FirstPerson);
		Add(TEXT("gun1p_first_person_layer"), M.Gun1P->FirstPersonPrimitiveType == EFP::FirstPerson);
		Add(TEXT("legs_world_layer"), LegsCount == 1 && Legs->FirstPersonPrimitiveType == EFP::None && Legs->IsBoneHiddenByName(TEXT("spine_01"))
			&& M.FP->IsBoneHiddenByName(TEXT("thigh_l")) && M.FP->IsBoneHiddenByName(TEXT("thigh_r")));
		Add(TEXT("legs_follow_1p_pose"), LegsCount == 1 && FVector::Dist(Legs->GetSocketLocation(TEXT("foot_l")), M.FP->GetSocketLocation(TEXT("foot_l"))) < 0.5);
		// scaled toward the camera, the muzzle stays inside the capsule (no wall clipping)
		Add(TEXT("gun_inside_capsule_when_scaled"), Muzzle * Scale < Radius);
		return Out;
	}

	bool GripLocked(const FMeshes& M)
	{
		const USceneComponent* Root = M.Gun1P ? M.Gun1P->GetOwner()->GetRootComponent() : nullptr;
		return Root && Root->GetAttachSocketName() == TEXT("hand_r");
	}

	// With -nullrhi nothing is rendered, and a skeletal mesh on the default AlwaysTickPose ticks its
	// animation but does not refresh its bones, so every socket read returned the un-IK'd pose.
	void AlwaysRefreshBones(UWorld* World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			for (USkeletalMeshComponent* Mesh : TInlineComponentArray<USkeletalMeshComponent*>(*It))
			{
				Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			}
		}
	}

	struct FStateSamples
	{
		FString Name;
		TArray<FSample> Samples;
	};

	// Largest per-axis range of a vector over the samples.
	template <typename TGetter>
	float Range(const TArray<FSample>& Ss, TGetter Get)
	{
		float Out = 0;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			float Lo = TNumericLimits<float>::Max(), Hi = TNumericLimits<float>::Lowest();
			for (const FSample& S : Ss) { const float V = Get(S)[Axis]; Lo = FMath::Min(Lo, V); Hi = FMath::Max(Hi, V); }
			Out = FMath::Max(Out, Hi - Lo);
		}
		return Out;
	}

	template <typename TGetter>
	float MaxOf(const TArray<FSample>& Ss, TGetter Get)
	{
		float Out = TNumericLimits<float>::Lowest();
		for (const FSample& S : Ss) { Out = FMath::Max(Out, (float)Get(S)); }
		return Out;
	}

	template <typename TGetter>
	float MinOf(const TArray<FSample>& Ss, TGetter Get)
	{
		return -MaxOf(Ss, [&Get](const FSample& S) { return -Get(S); });
	}

	bool IsMoving(const FString& Name) { return Name.Contains(TEXT("walk")) || Name.Contains(TEXT("strafe")); }
	bool IsTurning(const FString& Name) { return Name.StartsWith(TEXT("turn")); }

	// Only an elbow in front of the camera can be seen (x <= 10 is beside / behind it).
	bool ElbowVisible(const TArray<FSample>& Ss, bool bLeft)
	{
		return MaxOf(Ss, [bLeft](const FSample& S) { return (bLeft ? S.ElbowLView : S.ElbowRView).X; }) > 10.f;
	}

	/** Grades the sweep. Returns one "PASS name  detail" / "FAIL name  detail" line per check. */
	TArray<TPair<bool, FString>> Grade(const FStatic& Static, const TArray<FStateSamples>& Results)
	{
		TArray<TPair<bool, FString>> Lines;
		auto Check = [&Lines](const FString& Name, bool bOk, const FString& Detail) { Lines.Emplace(bOk, Name + TEXT("  ") + Detail); };

		for (const TPair<FString, bool>& C : Static.Checks)
		{
			Check(TEXT("static ") + C.Key, C.Value, C.Value ? TEXT("True") : TEXT("False"));
		}

		TArray<FSample> Every;
		const FStateSamples* Idle = nullptr;
		for (const FStateSamples& St : Results)
		{
			Every.Append(St.Samples);
			Idle = St.Name == TEXT("stand_0") ? &St : Idle;
		}

		for (const FStateSamples& St : Results)
		{
			const TArray<FSample>& Ss = St.Samples;
			const FString& N = St.Name;
			const bool bMoving = IsMoving(N);

			const FSample* Closest = &Ss[0];
			for (const FSample& S : Ss) { Closest = S.TorsoClearance < Closest->TorsoClearance ? &S : Closest; }
			Check(N + TEXT(" torso clear"), Closest->TorsoClearance >= Limits::TorsoClearanceMin,
				FString::Printf(TEXT("%.1fcm (%s)"), Closest->TorsoClearance, *Closest->TorsoClosestBone.ToString()));

			const float FeetGap = MaxOf(Ss, [](const FSample& S) { return S.FeetVsShadow; });
			Check(N + TEXT(" feet on shadow"), FeetGap <= (bMoving ? Limits::FeetMovingMax : Limits::FeetStaticMax), FString::Printf(TEXT("%.2fcm"), FeetGap));

			// absolute limit, unless the 3P animation itself bends the support wrist more in that frame
			const float WL = MaxOf(Ss, [](const FSample& S) { return S.WristBendL - FMath::Max(0.f, S.WristBendL3P - Limits::WristBendLMax); });
			const float DL = MaxOf(Ss, [](const FSample& S) { return S.WristBendL - S.WristBendL3P; });
			const float DR = MaxOf(Ss, [](const FSample& S) { return S.WristBendR - S.WristBendR3P; });
			Check(N + TEXT(" wrists"), WL <= Limits::WristBendLMax && FMath::Max(DL, DR) <= Limits::WristBendVs3PMax,
				FString::Printf(TEXT("L %.0f deg, vs 3P L %+.0f R %+.0f deg"), WL, DL, DR));

			auto FingerRange = [&Ss](bool b3P, int32* OutWorst)
			{
				float Out = 0;
				for (int32 J = 0; J < Ss[0].Fingers1P.Num(); ++J)
				{
					const float R = Range(Ss, [b3P, J](const FSample& S) { return (b3P ? S.Fingers3P : S.Fingers1P)[J]; });
					if (R > Out) { Out = R; if (OutWorst) { *OutWorst = J; } }
				}
				return Out;
			};
			int32 Worst = 0;
			const float FR = FingerRange(false, &Worst);
			Check(N + TEXT(" fingers keep the grip"), FR <= Limits::FingerRangeMax,
				FString::Printf(TEXT("%.2fcm (%s; 3P anim %.2fcm)"), FR, *FingerJoints()[Worst].ToString(), FingerRange(true, nullptr)));

			const float NC = MinOf(Ss, [](const FSample& S) { return S.BodyNearCamera; });
			Check(N + TEXT(" camera outside body"), NC >= Limits::BodyNearCameraMin, FString::Printf(TEXT("%.1fcm"), NC));
			const float SC = MinOf(Ss, [](const FSample& S) { return S.ShoulderNearCamera; });
			Check(N + TEXT(" shoulders away from camera"), SC >= Limits::ShoulderNearCameraMin, FString::Printf(TEXT("%.1fcm"), SC));
			const float EZ = MaxOf(Ss, [](const FSample& S) { return FMath::Max(S.ElbowLView.Z, S.ElbowRView.Z); });
			Check(N + TEXT(" elbows low"), EZ <= Limits::ElbowViewZMax, FString::Printf(TEXT("view z %.1f"), EZ));

			const float HL = Range(Ss, [](const FSample& S) { return S.HandLInGun; });
			const float HR = Range(Ss, [](const FSample& S) { return S.HandRInGun; });
			Check(N + TEXT(" hands stay on gun"), FMath::Max(HL, HR) <= Limits::HandInGunRangeMax, FString::Printf(TEXT("L %.2f R %.2f cm"), HL, HR));

			if (!bMoving && !N.Contains(TEXT("turn")))
			{
				const float MZ = Range(Ss, [](const FSample& S) { return S.MuzzleView; });
				Check(N + TEXT(" gun still on screen"), MZ <= Limits::MuzzleViewStaticRangeMax, FString::Printf(TEXT("%.2fcm"), MZ));
			}
		}

		if (Idle)
		{
			for (const FStateSamples& St : Results)
			{
				if (!IsTurning(St.Name)) { continue; }
				float Rise = 0;
				bool bAny = false;
				for (const bool bLeft : {true, false})
				{
					if (!ElbowVisible(St.Samples, bLeft)) { continue; }
					auto Z = [bLeft](const FSample& S) { return (bLeft ? S.ElbowLView : S.ElbowRView).Z; };
					const float R = MaxOf(St.Samples, Z) - MaxOf(Idle->Samples, Z);
					Rise = bAny ? FMath::Max(Rise, R) : R;
					bAny = true;
				}
				Check(St.Name + TEXT(" elbows stay down while turning"), Rise <= Limits::ElbowRiseVsIdleMax, FString::Printf(TEXT("%+.1fcm vs stand_0"), Rise));
			}
		}

		for (const FStateSamples& St : Results)
		{
			if (!IsMoving(St.Name) && !St.Name.Contains(TEXT("turn"))) { continue; }
			float Bob = 0, VsGun = 0;
			for (const bool bLeft : {true, false})
			{
				if (!ElbowVisible(St.Samples, bLeft)) { continue; }
				auto Z = [bLeft](const FSample& S) { return (bLeft ? S.ElbowLView : S.ElbowRView).Z; };
				Bob = FMath::Max(Bob, MaxOf(St.Samples, Z) - MinOf(St.Samples, Z));
				VsGun = FMath::Max(VsGun, Range(St.Samples, [bLeft](const FSample& S) { return bLeft ? S.ElbowLInGun : S.ElbowRInGun; }));
			}
			Check(St.Name + TEXT(" elbows steady"), Bob <= Limits::ElbowBobMax, FString::Printf(TEXT("%.1fcm up/down"), Bob));
			Check(St.Name + TEXT(" elbows move with the gun"), VsGun <= Limits::ElbowVsGunRangeMax, FString::Printf(TEXT("%.1fcm vs gun"), VsGun));
		}

		const float BD = Range(Every, [](const FSample& S) { return S.BarrelViewDeg; });
		Check(TEXT("all states barrel aligned with view"), BD <= Limits::BarrelViewRangeDegMax, FString::Printf(TEXT("%.2f deg"), BD));
		return Lines;
	}

	/** Shared between the latent commands of one test run. */
	struct FRun
	{
		bool bQuick = false;
		TArray<const FState*> Plan;
		TArray<FName> Joints = FingerJoints();
		double StartTime = 0;
		double PieRequestTime = 0;
		// sweep
		int32 StateIndex = 0;
		int32 Frame = 0;
		int32 Samples = 0;
		float StateTime = 0;
		float BaseYaw = 0;
		FVector Start = FVector::ZeroVector;
		bool bStarted = false;
		TOptional<FStatic> Static;
		TArray<FStateSamples> Results;
		FString Error;
	};

	/** Starts a one-player standalone PIE whatever the local PIE settings are (often a listen server
	 *  with clients for network checks). */
	void RequestStandalonePie()
	{
		FRequestPlaySessionParams Params;
		Params.WorldType = EPlaySessionWorldType::PlayInEditor;
		ULevelEditorPlaySettings* Settings = NewObject<ULevelEditorPlaySettings>();
		Settings->SetPlayNetMode(EPlayNetMode::PIE_Standalone);
		Settings->SetPlayNumberOfClients(1);
		Settings->bLaunchSeparateServer = false;
		Params.EditorPlaySettings = Settings;
		GEditor->RequestPlaySession(Params);
	}

	UWorld* PieWorld()
	{
		return GEditor ? GEditor->PlayWorld.Get() : nullptr;
	}

	/** One frame of the sweep; returns true when every state is done (or on error). */
	bool TickSweep(FRun& R)
	{
		UWorld* World = PieWorld();
		if (!World)
		{
			R.Error = TEXT("PIE ended");
			return true;
		}
		AlwaysRefreshBones(World);
		FMeshes M = Find(World);
		if (!M.Pawn)
		{
			R.Error = TEXT("pawn lost");
			return true;
		}
		if (!R.bStarted)
		{
			R.bStarted = true;
			R.BaseYaw = M.PC->GetControlRotation().Yaw;
			R.Start = M.Pawn->GetActorLocation();
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				if (It->Tags.Contains(StartTag) || It->GetName() == TEXT("PlayerStart_0"))
				{
					R.BaseYaw = It->GetActorRotation().Yaw;
					R.Start = It->GetActorLocation();
				}
			}
		}

		const FState& S = *R.Plan[R.StateIndex];
		const FString Name = S.Name;
		ACharacter* Character = Cast<ACharacter>(M.Pawn);
		if (R.Frame == 0)
		{
			// every state starts at the same spot, facing the same way
			M.Pawn->TeleportTo(R.Start, FRotator(0, R.BaseYaw, 0));
			M.PC->SetControlRotation(FRotator(S.Pitch, R.BaseYaw, 0));
			if (Character) { S.bCrouch ? Character->Crouch() : Character->UnCrouch(); }
			R.StateTime = 0;
		}
		// game time, so a fixed time step (-UseFixedTimeStep) runs faster than real time
		R.StateTime += World->GetDeltaSeconds();
		const bool bWarm = R.Frame >= WarmupFrames && R.StateTime >= WarmupSeconds;
		// What a player's mouse and keys end up calling: control rotation and AddMovementInput.
		M.PC->SetControlRotation(FRotator(S.Pitch, R.BaseYaw + S.TurnRate * R.StateTime, 0));
		if (!S.Move.IsZero())
		{
			const FRotator YawRot(0, R.BaseYaw, 0);
			M.Pawn->AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X) * S.Move.X + FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y) * S.Move.Y, 1.f, true);
		}
		// Reads land one frame after the input above: the pose and camera of the frame just ticked.
		if (bWarm && !Name.StartsWith(TEXT("_")))
		{
			if (!M.IsValid())
			{
				R.Error = TEXT("first-person meshes or weapon not found");
				return true;
			}
			if (!R.Static)
			{
				R.Static = StaticChecks(M);
			}
			if (R.Results.IsEmpty() || R.Results.Last().Name != Name)
			{
				R.Results.Add({Name, {}});
			}
			R.Results.Last().Samples.Add(Sample(M, R.Joints));
		}
		++R.Frame;
		if (bWarm)
		{
			++R.Samples;
		}
		if (bWarm && R.Samples >= SampleFrames)
		{
			if (Name == TEXT("_settle") && !GripLocked(M) && R.Frame < SettleMaxFrames)
			{
				return false;
			}
			R.Frame = 0;
			R.Samples = 0;
			if (++R.StateIndex >= R.Plan.Num())
			{
				if (Character) { Character->UnCrouch(); }
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FFpsFirstPersonRigTest, "FpsCore.FirstPersonRig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FFpsFirstPersonRigTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("Quick"));
	OutTestCommands.Add(TEXT("Quick"));
	OutBeautifiedNames.Add(TEXT("Full"));
	OutTestCommands.Add(TEXT("Full"));
}

bool FFpsFirstPersonRigTest::RunTest(const FString& Parameters)
{
	using namespace FpsRig;
	TSharedRef<FRun> R = MakeShared<FRun>();
	R->bQuick = Parameters == TEXT("Quick");
	for (const FState& S : States)
	{
		if (!R->bQuick || S.bQuick)
		{
			R->Plan.Add(&S);
		}
	}
	R->StartTime = FPlatformTime::Seconds();

	if (!FEditorFileUtils::LoadMap(FPackageName::LongPackageNameToFilename(TestMap, FPackageName::GetMapPackageExtension()), false, false))
	{
		AddError(FString::Printf(TEXT("Cannot load %s"), TestMap));
		return false;
	}
	RequestStandalonePie();
	R->PieRequestTime = FPlatformTime::Seconds();

	// 1. wait for the pawn (spawn, experience load, equip)
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, R]()
	{
		const FMeshes M = Find(PieWorld());
		if (M.Pawn)
		{
			AddInfo(FString::Printf(TEXT("pawn ready after %.1fs"), FPlatformTime::Seconds() - R->PieRequestTime));
			return true;
		}
		if (FPlatformTime::Seconds() - R->PieRequestTime > PawnTimeoutSeconds)
		{
			R->Error = TEXT("no pawn");
			return true;
		}
		return false;
	}));

	// 2. drive the states, one frame per update
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([R]()
	{
		if (!R->Error.IsEmpty())
		{
			return true;
		}
		if (FPlatformTime::Seconds() - R->StartTime > SweepTimeoutSeconds)
		{
			R->Error = TEXT("timeout");
			return true;
		}
		return TickSweep(*R);
	}));

	// 3. grade, report, stop PIE
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, R]()
	{
		TArray<FString> Report;
		if (!R->Error.IsEmpty())
		{
			AddError(TEXT("sweep did not finish: ") + R->Error);
			Report.Add(TEXT("FAIL sweep finished  ") + R->Error);
		}
		int32 Fails = Report.Num();
		if (R->Static && !R->Results.IsEmpty())
		{
			for (const TPair<bool, FString>& Line : Grade(*R->Static, R->Results))
			{
				Report.Add((Line.Key ? TEXT("PASS ") : TEXT("FAIL ")) + Line.Value);
				if (Line.Key)
				{
					AddInfo(Line.Value);
				}
				else
				{
					AddError(Line.Value);
					++Fails;
				}
			}
		}
		Report.Add(Fails ? FString::Printf(TEXT("%d FAIL"), Fails) : TEXT("ALL PASS"));
		AddInfo(FString::Printf(TEXT("%d states, %d checks, %.1fs"), R->Results.Num(), Report.Num() - 1, FPlatformTime::Seconds() - R->StartTime));
		// plain-text copy next to the automation report, easy to diff between runs
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("FpRigCheck");
		IFileManager::Get().MakeDirectory(*Dir, true);
		FFileHelper::SaveStringArrayToFile(Report, *(Dir / (R->bQuick ? TEXT("report_quick.txt") : TEXT("report_full.txt"))));
		GEditor->RequestEndPlayMap();
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([]() { return PieWorld() == nullptr; }));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
