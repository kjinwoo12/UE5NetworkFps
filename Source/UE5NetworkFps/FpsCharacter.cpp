// Fill out your copyright notice in the Description page of Project Settings.

#include "FpsCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AFpsCharacter::AFpsCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCapsuleSize(34.f, 96.f);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	check(CharacterMesh);

	CharacterMesh->SetRelativeLocation(FVector(-10.f, 0.f, -96.f));
	CharacterMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	CharacterMesh->SetOwnerNoSee(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> ThirdPersonAnimClass(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));

	if (CharacterMeshAsset.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(CharacterMeshAsset.Object);
	}

	if (ThirdPersonAnimClass.Succeeded())
	{
		CharacterMesh->SetAnimInstanceClass(ThirdPersonAnimClass.Class);
	}

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(CharacterMesh);
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->CastShadow = false;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FirstPersonMeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> FirstPersonAnimClass(
		TEXT("/Game/FirstPerson/Anims/ABP_FP_Copy.ABP_FP_Copy_C"));

	if (FirstPersonMeshAsset.Succeeded())
	{
		FirstPersonMesh->SetSkeletalMesh(FirstPersonMeshAsset.Object);
	}

	if (FirstPersonAnimClass.Succeeded())
	{
		FirstPersonMesh->SetAnimInstanceClass(FirstPersonAnimClass.Class);
	}

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(FirstPersonMesh);
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 10.f, 165.f));
	FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
	FirstPersonCamera->bEnableFirstPersonScale = true;
	FirstPersonCamera->FirstPersonFieldOfView = 70.f;
	FirstPersonCamera->FirstPersonScale = 0.7f;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->BrakingDecelerationFalling = 1500.f;
		Movement->AirControl = 0.6f;
	}
}

void AFpsCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// BP may serialize FirstPersonPrimitiveType=None and override the constructor default.
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOwnerNoSee(true);
		CharacterMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
		CharacterMesh->CastShadow = true;
	}
}

void AFpsCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFpsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFpsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
