// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "FpsFirstPersonSettings.generated.h"

class APawn;

/**
 * Which first-person body setup the local player uses. Project Settings > Game > Fps First Person
 * (DefaultGame.ini). Read by ULyraCameraMode_FirstPerson and UFpsFirstPersonAnimInstance, so the
 * camera and the body always agree.
 *
 * True first person: the owner sees the third-person animated body; the camera sits at the
 * animated head (eyes) and the head bone is hidden. Only the arms are re-solved so the weapon
 * stays fixed on screen. Feet, legs and torso are the 3P animation, so they match the shadow.
 *
 * Legacy (bTrueFirstPerson = false): camera at the capsule eye height; the 1P control rig pins the
 * head to the camera, turns the shoulders with pitch and pushes the body back when looking down.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Fps First Person"))
class UFpsFirstPersonSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UFpsFirstPersonSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, Category = "Body")
	bool bTrueFirstPerson = true;

	/** Bone on the pawn's animated mesh the camera follows (true first person). */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson"))
	FName EyeBone = TEXT("head");

	/** Eye position from EyeBone in view space (X forward, Y right, Z up, cm). */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson"))
	FVector EyeViewOffset = FVector(8.0, 0.0, 10.0);

	/**
	 * True first person: how much the weapon follows the way the animated body carries it (bob,
	 * turn lag, aim offset), 0 = locked to the view at the grip profile's TrueFirstPersonWeaponViewOffset.
	 * A locked weapon made the arms absorb every body motion (elbows jerking up on fast turns, bent
	 * wrists while walking). The motion is limited by WeaponSwayMaxOffset / MaxAngle and smoothed.
	 * The full carry (1) swung the muzzle ~19cm sideways walking backwards, and the arms with it.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", ClampMax = "1.0"))
	float WeaponFollowsBody = 0.25f;

	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", Units = "cm"))
	float WeaponSwayMaxOffset = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", ClampMax = "45.0", Units = "deg"))
	float WeaponSwayMaxAngle = 6.0f;

	/** Interp speed toward the body's carry (higher = tighter, 0 = no smoothing). */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float WeaponSwaySmoothing = 15.0f;

	/** The sway is measured from the body carry's slow average (VInterpTo speed): lower keeps slower motion. */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float WeaponSwayCenterSpeed = 2.0f;

	/**
	 * The weapon follows the body's carry fully from this ground speed (cm/s) and not at all standing
	 * still, so posture changes in place (crouch, looking down) leave it where the profile frames it.
	 * 0 = always follow.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float WeaponSwayFullSpeed = 150.0f;

	/** Turn inertia: degrees the weapon trails the view per 100 deg/s of view rotation (yaw and pitch). */
	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float WeaponTurnLag = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Body", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", ClampMax = "20.0", Units = "deg"))
	float WeaponTurnLagMax = 2.0f;

	/**
	 * Elbow pole blend per arm: 0 = the animated elbow (relative to the hand), 1 = the elbow that
	 * keeps the wrist straight. Higher straightens the wrist but raises the elbow into view.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PrimaryWristStraighten = 0.5f;

	/** True first person: PrimaryWristStraighten while moving (blended by speed up to WeaponSwayFullSpeed), where PrimaryShoulderFitsElbow lets the elbow reach the pole. */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PrimaryWristStraightenMoving = 0.75f;

	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SupportWristStraighten = 1.0f;

	/**
	 * The elbow poles follow the animation through a low-pass filter in weapon space (VInterpTo speed):
	 * the walk cycle swung the animated elbow and with it the arm's bend plane. 0 = no filtering.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (ClampMin = "0.0"))
	float ElbowPoleSmoothingSpeed = 3.0f;

	/**
	 * Finger bones (with everything below them) that keep the pose they had when the grip was captured,
	 * as long as no montage plays: the locomotion animations open and close the fingers (the thumb most)
	 * with every step, which a first-person view right in front of the hands shows plainly.
	 * Empty = the fingers follow the animation.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms")
	TArray<FName> LockedFingerRoots = {
		TEXT("thumb_01_l"), TEXT("index_metacarpal_l"), TEXT("middle_metacarpal_l"), TEXT("ring_metacarpal_l"), TEXT("pinky_metacarpal_l"),
		TEXT("thumb_01_r"), TEXT("index_metacarpal_r"), TEXT("middle_metacarpal_r"), TEXT("ring_metacarpal_r"), TEXT("pinky_metacarpal_r") };

	/** How fast the fingers blend into / out of the captured grip (alpha per second; 0 = at once), e.g. around a reload. */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (ClampMin = "0.0"))
	float FingerLockBlendSpeed = 10.0f;

	/**
	 * True first person: an arm counts as out of reach past this fraction of its length; the
	 * clavicle then turns toward the hand target (CR_Mannequin_FirstPerson "Reach") to bring the
	 * shoulder forward, as a person does, instead of the arm locking straight and the hand sliding
	 * off the gun when locomotion sways the shoulders.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.5", ClampMax = "1.0"))
	float ReachArmFraction = 0.95f;

	/**
	 * True first person: the shoulders (upperarm bones) are kept at least this far from the camera,
	 * both when reaching and when an animation brings them close; nearer, the shoulder mesh
	 * cuts the near clip plane at the screen edge. Reaching also never moves a shoulder sideways
	 * toward the view axis.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", Units = "cm"))
	float MinShoulderCameraDistance = 24.0f;

	/**
	 * True first person: the shoulders follow the animation through a low-pass filter in view space
	 * (VInterpTo speed): the walk/turn bob of the animated shoulders made the elbows jerk up and down
	 * while the hands hold the weapon. 0 = no filtering.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float ShoulderSmoothingSpeed = 3.0f;

	/**
	 * True first person: how far the primary shoulder moves (along the line to the elbow pole) so it
	 * sits one upper arm from the pole and the elbow lands on it. 0 = the shoulder stays where the
	 * filter puts it. Walking, the animation straightens the wrist by changing the arm around the hand;
	 * with the shoulder left alone the 1P elbow missed the pole and the wrist bent 10-25 degrees more
	 * than in third person.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", ClampMax = "1.0"))
	float PrimaryShoulderFitsElbow = 1.0f;

	/** Share of that shoulder move done by moving the chest (MaxChestOffset); the clavicle does the rest. More brings the collar toward the camera. */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", ClampMax = "1.0"))
	float PrimaryShoulderFitChestShare = 0.25f;

	/** Low-pass (VInterpTo speed) of that shoulder move: the slow crouch-walk cycle passed through it into the chest and the support elbow. */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float PrimaryShoulderFitSmoothingSpeed = 1.0f;

	/**
	 * True first person: the 1P chest (spine_04 and above) is moved by up to this much so the shoulders
	 * sit where the arms need them: low-passed in view space, carried along with the weapon sway and held
	 * during fast turns. The arms then keep their shape and move with the weapon as one piece instead of
	 * the elbows bobbing as the animated shoulders move under a steady gun. 0 disables.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", Units = "cm"))
	float MaxChestOffset = 15.0f;

	/** True first person: the chest offset never brings these bones closer to the camera than MinChestCameraDistance (nor closer than the animation has them). */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", Units = "cm"))
	float MinChestCameraDistance = 14.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson"))
	TArray<FName> ChestCameraClearanceBones = { TEXT("neck_01"), TEXT("neck_02"), TEXT("spine_05"), TEXT("clavicle_l"), TEXT("clavicle_r") };

	/**
	 * True first person: fast turns leave the animated upper body facing behind the view and leaning
	 * into the turn (the support elbow then rises into the lower left of the screen). The 1P chest is
	 * turned so its shoulder line keeps its captured direction in the view's heading frame, by up to this many
	 * degrees. 0 disables. Legs and the shadow keep the animation.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float MaxChestYawCorrection = 90.0f;

	/** The chest correction only applies while the view turns: from this rate (deg/s) up ... */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float ChestTurnRateStart = 60.0f;

	/** ... to full at this rate (deg/s). */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float ChestTurnRateFull = 180.0f;

	/** How fast the correction fades once the turn stops (FInterpTo speed), as the body catches up. */
	UPROPERTY(Config, EditAnywhere, Category = "Arms", meta = (EditCondition = "bTrueFirstPerson", ClampMin = "0.0"))
	float ChestCorrectionReleaseSpeed = 4.0f;

	/**
	 * Engine first person rendering (UE 5.5+): the owner's arms, upper body and weapon are drawn as
	 * "First Person" primitives, scaled toward the camera by FirstPersonScale, so they never sink
	 * into walls or the owner's own legs (which stay in the world layer). The third-person body is a
	 * World Space Representation: hidden from the owner, casting the owner's shadow.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Rendering", meta = (EditCondition = "bTrueFirstPerson"))
	bool bFirstPersonRendering = true;

	/** Scale of first person primitives toward the camera. The muzzle (~95cm out) must end up inside the capsule. */
	UPROPERTY(Config, EditAnywhere, Category = "Rendering", meta = (EditCondition = "bFirstPersonRendering", ClampMin = "0.01", ClampMax = "1.0"))
	float FirstPersonScale = 0.3f;

	/** Horizontal FOV for first person primitives; 0 = same as the scene (no viewmodel FOV). */
	UPROPERTY(Config, EditAnywhere, Category = "Rendering", meta = (EditCondition = "bFirstPersonRendering", ClampMin = "0.0", ClampMax = "170.0"))
	float FirstPersonFieldOfView = 0.0f;

	/** Bones whose subtree is drawn in the world layer (legs), by a copy of the 1P body. Hidden on the first person body. */
	UPROPERTY(Config, EditAnywhere, Category = "Rendering", meta = (EditCondition = "bFirstPersonRendering"))
	TArray<FName> WorldLayerBones = { TEXT("thigh_l"), TEXT("thigh_r") };

	/** Bones whose subtree is hidden on the world layer copy (everything drawn in the first person layer). */
	UPROPERTY(Config, EditAnywhere, Category = "Rendering", meta = (EditCondition = "bFirstPersonRendering"))
	TArray<FName> FirstPersonLayerBones = { TEXT("spine_01") };

	bool UseFirstPersonRendering() const { return bTrueFirstPerson && bFirstPersonRendering; }

	/**
	 * Eye location for true first person, from the pawn's own animated mesh.
	 * Returns false (OutLocation untouched) when the mode is off or the bone is missing.
	 */
	static bool GetTrueFirstPersonEyeLocation(const APawn* Pawn, const FRotator& ViewRotation, FVector& OutLocation);
};
