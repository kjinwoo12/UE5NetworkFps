// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsFirstPersonWeaponComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Weapons/LyraWeaponInstance.h"
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

	static bool TryGetLeftHandFallbackComponentSpace(
		const USkeletalMeshComponent* WeaponMesh,
		const USkeletalMeshComponent* FirstPersonMesh,
		const FVector& WeaponLocalOffset,
		FVector& OutLocationCS)
	{
		if (!WeaponMesh || !FirstPersonMesh)
		{
			return false;
		}

		const FName ReferenceSockets[] = { FName(TEXT("Hand_R")), FName(TEXT("Grip_R")), FName(TEXT("Grip")) };
		for (FName SocketName : ReferenceSockets)
		{
			if (!WeaponMesh->DoesSocketExist(SocketName))
			{
				continue;
			}

			const FTransform SocketWorld = WeaponMesh->GetSocketTransform(SocketName, RTS_World);
			const FVector TargetWorld = SocketWorld.GetLocation() + SocketWorld.TransformVectorNoScale(WeaponLocalOffset);
			OutLocationCS = FirstPersonMesh->GetComponentTransform().InverseTransformPosition(TargetWorld);
			return true;
		}

		return false;
	}
}

UFpsFirstPersonWeaponComponent::UFpsFirstPersonWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	SetIsReplicatedByDefault(false);
}

void UFpsFirstPersonWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	SyncFirstPersonWeaponVisuals();
}

void UFpsFirstPersonWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	if (USkeletalMeshComponent* FirstPersonMesh = FindFirstPersonMesh())
	{
		EnsureTickPrerequisites(FirstPersonMesh);
	}

	SyncFirstPersonWeaponVisuals();
}

void UFpsFirstPersonWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ApplyWeaponHandIk(false, false, nullptr);
	bLastAnyHandIkEnabled = false;
	ClearLocalVisuals();
	Super::EndPlay(EndPlayReason);
}

void UFpsFirstPersonWeaponComponent::EnsureTickPrerequisites(USkeletalMeshComponent* FirstPersonMesh)
{
	if (bCachedTickPrerequisites || !FirstPersonMesh)
	{
		return;
	}

	PrimaryComponentTick.AddPrerequisite(FirstPersonMesh, FirstPersonMesh->PrimaryComponentTick);

	if (USceneComponent* AttachParent = FirstPersonMesh->GetAttachParent())
	{
		FirstPersonMesh->AddTickPrerequisiteComponent(AttachParent);
	}

	bCachedTickPrerequisites = true;
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

void UFpsFirstPersonWeaponComponent::ApplyWeaponHandIk(bool bEnableLeftHandIk, bool bEnableRightHandIk, AActor* LocalWeaponVisual)
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

	const bool bAnyHandIk = bEnableLeftHandIk || bEnableRightHandIk;
	const bool bDisableRightHandIk = !bEnableRightHandIk;
	const bool bDisableLeftHandIk = !bEnableLeftHandIk;

	if (bAnyHandIk != bLastAnyHandIkEnabled)
	{
		SetAnimInstanceBool(AnimInstance, TEXT("bWeaponEquipped"), bAnyHandIk);
		bLastAnyHandIkEnabled = bAnyHandIk;
	}

	if (bDisableRightHandIk != bLastDisableRightHandIk)
	{
		SetAnimInstanceBool(AnimInstance, TEXT("DisableRHandIK"), bDisableRightHandIk);
		bLastDisableRightHandIk = bDisableRightHandIk;
	}

	if (bDisableLeftHandIk != bLastDisableLeftHandIk)
	{
		SetAnimInstanceBool(AnimInstance, TEXT("DisableLHandIK"), bDisableLeftHandIk);
		bLastDisableLeftHandIk = bDisableLeftHandIk;
	}

	if (!bAnyHandIk || !LocalWeaponVisual)
	{
		return;
	}

	USkeletalMeshComponent* WeaponMesh = FpsFirstPersonWeapon_Private::FindWeaponMesh(LocalWeaponVisual);
	if (!WeaponMesh)
	{
		return;
	}

	auto SetVectorIfChanged = [AnimInstance](FVector& LastValue, FName PropertyName, const FVector& NewValue)
	{
		if (!LastValue.Equals(NewValue, KINDA_SMALL_NUMBER))
		{
			LastValue = NewValue;
			SetAnimInstanceVector(AnimInstance, PropertyName, NewValue);
		}
	};

	if (bEnableRightHandIk)
	{
		const FName RightCandidates[] = { RightHandSocketName, FName(TEXT("Hand_R")), FName(TEXT("Grip_R")), FName(TEXT("Grip")) };
		FVector RightEffectorCS = FVector::ZeroVector;
		bool bHasRight = false;

		for (FName Candidate : RightCandidates)
		{
			if (FpsFirstPersonWeapon_Private::TryGetSocketComponentSpace(WeaponMesh, FirstPersonMesh, Candidate, RightEffectorCS))
			{
				bHasRight = true;
				break;
			}
		}

		if (!bHasRight)
		{
			RightEffectorCS = FirstPersonMesh->GetSocketTransform(TEXT("weapon_r"), RTS_Component).GetLocation();
		}

		const FVector RightJointCS = FirstPersonMesh->GetSocketTransform(TEXT("lowerarm_r"), RTS_Component).GetLocation();
		SetVectorIfChanged(LastEffectorLocation_R, TEXT("EffectorLocation_R"), RightEffectorCS);
		SetVectorIfChanged(LastJointTarget_R, TEXT("JointTarget_R"), RightJointCS);
	}

	if (bEnableLeftHandIk)
	{
		const FName LeftCandidates[] = { LeftHandSocketName, FName(TEXT("Hand_L")), FName(TEXT("Grip_L")) };
		FVector LeftEffectorCS = FVector::ZeroVector;
		bool bHasLeft = false;

		for (FName Candidate : LeftCandidates)
		{
			if (FpsFirstPersonWeapon_Private::TryGetSocketComponentSpace(WeaponMesh, FirstPersonMesh, Candidate, LeftEffectorCS))
			{
				bHasLeft = true;
				break;
			}
		}

		if (!bHasLeft)
		{
			bHasLeft = FpsFirstPersonWeapon_Private::TryGetLeftHandFallbackComponentSpace(
				WeaponMesh,
				FirstPersonMesh,
				LeftHandGripFallbackOffset,
				LeftEffectorCS);
		}

		if (!bHasLeft)
		{
			LeftEffectorCS = FirstPersonMesh->GetSocketTransform(TEXT("weapon_r"), RTS_Component).GetLocation()
				+ FVector(0.f, 12.f, 4.f);
		}

		const FVector LeftJointCS = FirstPersonMesh->GetSocketTransform(TEXT("lowerarm_l"), RTS_Component).GetLocation();
		SetVectorIfChanged(LastEffectorLocation_L, TEXT("EffectorLocation_L"), LeftEffectorCS);
		SetVectorIfChanged(LastJointTarget_L, TEXT("JointTarget_L"), LeftJointCS);
	}
}

bool UFpsFirstPersonWeaponComponent::HasActiveWeaponAnimLayer()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}

	ULyraEquipmentManagerComponent* EquipmentManager =
		Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		return false;
	}

	ULyraWeaponInstance* WeaponInstance = EquipmentManager->GetFirstInstanceOfType<ULyraWeaponInstance>();
	if (!WeaponInstance)
	{
		return false;
	}

	return WeaponInstance->PickBestAnimLayer(/*bEquipped=*/true, FGameplayTagContainer()) != nullptr;
}

bool UFpsFirstPersonWeaponComponent::ShouldApplyLeftHandIk(AActor* LocalWeaponVisual) const
{
	return bApplyLeftHandIk && LocalWeaponVisual != nullptr;
}

bool UFpsFirstPersonWeaponComponent::ShouldApplyRightHandIk(AActor* LocalWeaponVisual)
{
	if (!bApplyRightHandIk || !LocalWeaponVisual)
	{
		return false;
	}

	if (!bSkipRightHandIkWhenWeaponAnimLayerActive)
	{
		return true;
	}

	return !HasActiveWeaponAnimLayer();
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
		ApplyWeaponHandIk(false, false, nullptr);
		bLastAnyHandIkEnabled = false;
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
	const bool bEnableLeftHandIk = ShouldApplyLeftHandIk(PrimaryVisual);
	const bool bEnableRightHandIk = ShouldApplyRightHandIk(PrimaryVisual);
	ApplyWeaponHandIk(
		bEnableLeftHandIk,
		bEnableRightHandIk,
		(bEnableLeftHandIk || bEnableRightHandIk) ? PrimaryVisual : nullptr);
}
