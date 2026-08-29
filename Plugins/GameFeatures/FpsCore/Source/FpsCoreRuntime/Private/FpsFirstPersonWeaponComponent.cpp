// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsFirstPersonWeaponComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsFirstPersonWeaponComponent)

namespace FpsFirstPersonWeapon_Private
{
	static USkeletalMeshComponent* FindWeaponMesh(AActor* WeaponActor)
	{
		if (!WeaponActor)
		{
			return nullptr;
		}

		TInlineComponentArray<USkeletalMeshComponent*> Meshes(WeaponActor);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->GetSkeletalMeshAsset())
			{
				return Mesh;
			}
		}
		return Meshes.Num() > 0 ? Meshes[0] : nullptr;
	}

	static bool TryGetSocketComponentSpace(
		const USkeletalMeshComponent* WeaponMesh,
		const USkeletalMeshComponent* FirstPersonMesh,
		FName SocketName,
		FVector& OutLocationCS)
	{
		if (!WeaponMesh || !FirstPersonMesh || SocketName.IsNone())
		{
			return false;
		}

		if (!WeaponMesh->DoesSocketExist(SocketName))
		{
			return false;
		}

		const FTransform SocketWorld = WeaponMesh->GetSocketTransform(SocketName, RTS_World);
		OutLocationCS = FirstPersonMesh->GetComponentTransform().InverseTransformPosition(SocketWorld.GetLocation());
		return true;
	}
}

UFpsFirstPersonWeaponComponent::UFpsFirstPersonWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UFpsFirstPersonWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// Do not gate on IsLocallyControlled here — BeginPlay often runs before Possess.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SyncTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::SyncFirstPersonWeaponVisuals),
			0.05f,
			/*bLoop=*/true);
	}

	SyncFirstPersonWeaponVisuals();
}

void UFpsFirstPersonWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SyncTimerHandle);
	}

	ApplyWeaponHandIk(false, nullptr);
	bLastHandIkEnabled = false;
	ClearLocalVisuals();
	Super::EndPlay(EndPlayReason);
}

void UFpsFirstPersonWeaponComponent::SetAnimInstanceBool(UAnimInstance* AnimInstance, FName PropertyName, bool bValue)
{
	if (!AnimInstance)
	{
		return;
	}

	if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), PropertyName))
	{
		BoolProperty->SetPropertyValue_InContainer(AnimInstance, bValue);
	}
}

void UFpsFirstPersonWeaponComponent::SetAnimInstanceVector(UAnimInstance* AnimInstance, FName PropertyName, const FVector& Value)
{
	if (!AnimInstance)
	{
		return;
	}

	if (FStructProperty* StructProperty = FindFProperty<FStructProperty>(AnimInstance->GetClass(), PropertyName))
	{
		if (StructProperty->Struct == TBaseStructure<FVector>::Get())
		{
			*StructProperty->ContainerPtrToValuePtr<FVector>(AnimInstance) = Value;
		}
	}
}

USkeletalMeshComponent* UFpsFirstPersonWeaponComponent::FindFirstPersonMesh() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	auto IsFirstPersonMesh = [](const USkeletalMeshComponent* Mesh) -> bool
	{
		if (!Mesh)
		{
			return false;
		}

		const FString Name = Mesh->GetName();
		if (Name.Contains(TEXT("FirstPerson"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		return Mesh->bOnlyOwnerSee && !Mesh->bOwnerNoSee;
	};

	TArray<UChildActorComponent*> ChildActorComponents;
	OwnerActor->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		AActor* PartActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (!PartActor)
		{
			continue;
		}

		TArray<USkeletalMeshComponent*> Meshes;
		PartActor->GetComponents<USkeletalMeshComponent>(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (IsFirstPersonMesh(Mesh))
			{
				return Mesh;
			}
		}
	}

	TArray<USkeletalMeshComponent*> AllMeshes;
	OwnerActor->GetComponents<USkeletalMeshComponent>(AllMeshes, /*bIncludeFromChildActors=*/true);
	USkeletalMeshComponent* CharacterMesh = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(OwnerActor))
	{
		CharacterMesh = Character->GetMesh();
	}
	for (USkeletalMeshComponent* Mesh : AllMeshes)
	{
		if (IsFirstPersonMesh(Mesh) && Mesh != CharacterMesh)
		{
			return Mesh;
		}
	}

	return nullptr;
}

void UFpsFirstPersonWeaponComponent::ClearLocalVisuals()
{
	for (AActor* Visual : LocalWeaponVisuals)
	{
		if (IsValid(Visual))
		{
			Visual->Destroy();
		}
	}
	LocalWeaponVisuals.Reset();
}

void UFpsFirstPersonWeaponComponent::ApplyWeaponHandIk(bool bEnableHandIk, AActor* LocalWeaponVisual) const
{
	USkeletalMeshComponent* FirstPersonMesh = FindFirstPersonMesh();
	if (!FirstPersonMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = FirstPersonMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	SetAnimInstanceBool(AnimInstance, TEXT("bWeaponEquipped"), bEnableHandIk);

	if (!bEnableHandIk || !LocalWeaponVisual)
	{
		return;
	}

	USkeletalMeshComponent* WeaponMesh = FpsFirstPersonWeapon_Private::FindWeaponMesh(LocalWeaponVisual);
	if (!WeaponMesh)
	{
		return;
	}

	const FName RightCandidates[] = { RightHandSocketName, FName(TEXT("Hand_R")), FName(TEXT("Grip_R")), FName(TEXT("Grip")) };
	const FName LeftCandidates[] = { LeftHandSocketName, FName(TEXT("Hand_L")), FName(TEXT("Grip_L")) };

	FVector RightEffectorCS = FVector::ZeroVector;
	FVector LeftEffectorCS = FVector::ZeroVector;
	bool bHasRight = false;
	bool bHasLeft = false;

	for (FName Candidate : RightCandidates)
	{
		if (FpsFirstPersonWeapon_Private::TryGetSocketComponentSpace(WeaponMesh, FirstPersonMesh, Candidate, RightEffectorCS))
		{
			bHasRight = true;
			break;
		}
	}
	for (FName Candidate : LeftCandidates)
	{
		if (FpsFirstPersonWeapon_Private::TryGetSocketComponentSpace(WeaponMesh, FirstPersonMesh, Candidate, LeftEffectorCS))
		{
			bHasLeft = true;
			break;
		}
	}

	if (!bHasRight)
	{
		// Fallback: weapon_r bone on the FP mesh (attach socket).
		RightEffectorCS = FirstPersonMesh->GetSocketTransform(TEXT("weapon_r"), RTS_Component).GetLocation();
		bHasRight = true;
	}
	if (!bHasLeft)
	{
		LeftEffectorCS = RightEffectorCS + FVector(0.f, 12.f, 4.f);
		bHasLeft = true;
	}

	// Elbow hints: pull slightly toward the body so the chain bends naturally.
	const FVector RightJointCS = FirstPersonMesh->GetSocketTransform(TEXT("lowerarm_r"), RTS_Component).GetLocation();
	const FVector LeftJointCS = FirstPersonMesh->GetSocketTransform(TEXT("lowerarm_l"), RTS_Component).GetLocation();

	SetAnimInstanceVector(AnimInstance, TEXT("EffectorLocation_R"), RightEffectorCS);
	SetAnimInstanceVector(AnimInstance, TEXT("EffectorLocation_L"), LeftEffectorCS);
	SetAnimInstanceVector(AnimInstance, TEXT("JointTarget_R"), RightJointCS);
	SetAnimInstanceVector(AnimInstance, TEXT("JointTarget_L"), LeftJointCS);
}

void UFpsFirstPersonWeaponComponent::SyncFirstPersonWeaponVisuals()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	ULyraEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		return;
	}

	TArray<ULyraEquipmentInstance*> EquipmentInstances =
		EquipmentManager->GetEquipmentInstancesOfType(ULyraEquipmentInstance::StaticClass());

	TArray<AActor*> ReplicatedWeapons;
	for (ULyraEquipmentInstance* Instance : EquipmentInstances)
	{
		if (!Instance)
		{
			continue;
		}

		for (AActor* Spawned : Instance->GetSpawnedActors())
		{
			if (IsValid(Spawned))
			{
				ReplicatedWeapons.Add(Spawned);
			}
		}
	}

	USkeletalMeshComponent* FirstPersonMesh = FindFirstPersonMesh();

	int32 Signature = ReplicatedWeapons.Num();
	for (AActor* WeaponActor : ReplicatedWeapons)
	{
		Signature ^= GetTypeHash(WeaponActor);
		Signature ^= GetTypeHash(WeaponActor->GetAttachParentSocketName());
	}
	if (FirstPersonMesh)
	{
		Signature ^= GetTypeHash(FirstPersonMesh);
	}

	const bool bReady =
		FirstPersonMesh != nullptr
		&& ReplicatedWeapons.Num() > 0
		&& ReplicatedWeapons.ContainsByPredicate([](const AActor* WeaponActor)
		{
			return WeaponActor && !WeaponActor->GetAttachParentSocketName().IsNone();
		});

	if (!bReady)
	{
		if (LocalWeaponVisuals.Num() > 0)
		{
			ClearLocalVisuals();
		}
		ApplyWeaponHandIk(false, nullptr);
		bLastHandIkEnabled = false;
		LastSyncSignature = 0;
		return;
	}

	if (Signature != LastSyncSignature || LocalWeaponVisuals.Num() != ReplicatedWeapons.Num())
	{
		LastSyncSignature = Signature;
		ClearLocalVisuals();

		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		for (AActor* WeaponActor : ReplicatedWeapons)
		{
			const FName AttachSocket = WeaponActor->GetAttachParentSocketName();
			if (AttachSocket.IsNone())
			{
				continue;
			}

			const FTransform RelativeXf = WeaponActor->GetRootComponent()
				? WeaponActor->GetRootComponent()->GetRelativeTransform()
				: FTransform::Identity;

			TInlineComponentArray<UPrimitiveComponent*> Primitives(WeaponActor);
			for (UPrimitiveComponent* Primitive : Primitives)
			{
				if (Primitive)
				{
					Primitive->SetOwnerNoSee(true);
				}
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Pawn;
			SpawnParams.Instigator = Pawn;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;
			SpawnParams.bDeferConstruction = true;

			AActor* Visual = World->SpawnActor<AActor>(WeaponActor->GetClass(), FTransform::Identity, SpawnParams);
			if (!Visual)
			{
				continue;
			}

			Visual->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/true);
			Visual->SetReplicates(false);
			Visual->SetActorEnableCollision(false);

			Visual->AttachToComponent(
				FirstPersonMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				AttachSocket);
			Visual->SetActorRelativeTransform(RelativeXf);

			TInlineComponentArray<UPrimitiveComponent*> VisualPrimitives(Visual);
			for (UPrimitiveComponent* Primitive : VisualPrimitives)
			{
				if (Primitive)
				{
					Primitive->SetOnlyOwnerSee(true);
					Primitive->SetOwnerNoSee(false);
					Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Primitive->SetVisibility(true, true);
				}
			}

			LocalWeaponVisuals.Add(Visual);
		}
	}

	AActor* PrimaryVisual = LocalWeaponVisuals.Num() > 0 ? LocalWeaponVisuals[0].Get() : nullptr;
	ApplyWeaponHandIk(PrimaryVisual != nullptr, PrimaryVisual);
	bLastHandIkEnabled = PrimaryVisual != nullptr;
}
