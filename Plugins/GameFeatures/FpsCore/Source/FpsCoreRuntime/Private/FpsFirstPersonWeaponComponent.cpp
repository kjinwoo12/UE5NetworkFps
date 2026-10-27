// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsFirstPersonWeaponComponent.h"

#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Cosmetics/LyraPawnComponent_CharacterParts.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "FpsFirstPersonAnimInstance.h"
#include "FpsFirstPersonSettings.h"
#include "FpsWeaponGripComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsFirstPersonWeaponComponent)

UFpsFirstPersonWeaponComponent::UFpsFirstPersonWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UFpsFirstPersonWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// Do not gate on IsLocallyControlled here — BeginPlay often runs before Possess.
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandleControllerChanged);
		BindPawnSources(Pawn);
	}

#if WITH_EDITOR
	SettingsChangedHandle = GetMutableDefault<UFpsFirstPersonSettings>()->OnSettingChanged().AddUObject(this, &ThisClass::HandleSettingsChanged);
#endif

	SyncFirstPersonWeaponVisuals();
}

void UFpsFirstPersonWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAll();
	ApplyWeaponGrip(nullptr);
	ClearLocalVisuals();
	DestroyWorldLayerLegs();
	Super::EndPlay(EndPlayReason);
}

void UFpsFirstPersonWeaponComponent::DestroyWorldLayerLegs()
{
	if (WorldLayerLegs)
	{
		WorldLayerLegs->DestroyComponent();
		WorldLayerLegs = nullptr;
	}
}

void UFpsFirstPersonWeaponComponent::RequestSync()
{
	UWorld* World = GetWorld();
	if (bSyncRequested || !World || !HasBegunPlay())
	{
		return;
	}

	// Next tick: removals are broadcast before the entry is gone, and attaching from inside an
	// animation update (OnGripLocked) is avoided.
	bSyncRequested = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::SyncFirstPersonWeaponVisuals));
}

void UFpsFirstPersonWeaponComponent::BindPawnSources(APawn* Pawn)
{
	if (!BoundEquipmentManager.IsValid())
	{
		if (ULyraEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>())
		{
			BoundEquipmentManager = EquipmentManager;
			EquipmentChangedHandle = EquipmentManager->OnEquipmentChanged.AddUObject(this, &ThisClass::HandleEquipmentChanged);
		}
	}

	if (!BoundCharacterParts.IsValid())
	{
		if (ULyraPawnComponent_CharacterParts* CharacterParts = Pawn->FindComponentByClass<ULyraPawnComponent_CharacterParts>())
		{
			BoundCharacterParts = CharacterParts;
			CharacterParts->OnCharacterPartsChanged.AddUniqueDynamic(this, &ThisClass::HandleCharacterPartsChanged);
		}
	}
}

void UFpsFirstPersonWeaponComponent::BindFirstPersonMesh(USkeletalMeshComponent* FirstPersonMesh)
{
	if (BoundFirstPersonMesh.Get() != FirstPersonMesh)
	{
		if (USkeletalMeshComponent* OldMesh = BoundFirstPersonMesh.Get())
		{
			OldMesh->OnAnimInitialized.RemoveDynamic(this, &ThisClass::HandleFirstPersonAnimInitialized);
		}
		BoundFirstPersonMesh = FirstPersonMesh;
		if (FirstPersonMesh)
		{
			FirstPersonMesh->OnAnimInitialized.AddUniqueDynamic(this, &ThisClass::HandleFirstPersonAnimInitialized);
		}
	}

	UFpsFirstPersonAnimInstance* AnimInstance = FirstPersonMesh ? Cast<UFpsFirstPersonAnimInstance>(FirstPersonMesh->GetAnimInstance()) : nullptr;
	if (BoundAnimInstance.Get() != AnimInstance)
	{
		if (UFpsFirstPersonAnimInstance* OldAnimInstance = BoundAnimInstance.Get())
		{
			OldAnimInstance->OnGripLocked.Remove(GripLockedHandle);
		}
		GripLockedHandle.Reset();
		BoundAnimInstance = AnimInstance;
		if (AnimInstance)
		{
			GripLockedHandle = AnimInstance->OnGripLocked.AddUObject(this, &ThisClass::RequestSync);
		}
	}
}

void UFpsFirstPersonWeaponComponent::WatchReplicatedWeapons(const TArray<AActor*>& ReplicatedWeapons)
{
	for (const TWeakObjectPtr<AActor>& Watched : WatchedWeapons)
	{
		if (AActor* WeaponActor = Watched.Get())
		{
			WeaponActor->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleWeaponEndPlay);
		}
	}
	WatchedWeapons.Reset();

	for (const TPair<TWeakObjectPtr<USceneComponent>, FDelegateHandle>& Pending : PendingAttachRoots)
	{
		if (USceneComponent* Root = Pending.Key.Get())
		{
			Root->TransformUpdated.Remove(Pending.Value);
		}
	}
	PendingAttachRoots.Reset();

	for (AActor* WeaponActor : ReplicatedWeapons)
	{
		WeaponActor->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleWeaponEndPlay);
		WatchedWeapons.Add(WeaponActor);

		// On clients the weapon can replicate before its attachment; attaching moves its root.
		USceneComponent* Root = WeaponActor->GetRootComponent();
		if (Root && !Root->GetAttachParent())
		{
			PendingAttachRoots.Emplace(Root, Root->TransformUpdated.AddUObject(this, &ThisClass::HandleWeaponTransformUpdated));
		}
	}
}

void UFpsFirstPersonWeaponComponent::UnbindAll()
{
	bSyncRequested = false;
	WatchReplicatedWeapons({});
	BindFirstPersonMesh(nullptr);

	if (ULyraEquipmentManagerComponent* EquipmentManager = BoundEquipmentManager.Get())
	{
		EquipmentManager->OnEquipmentChanged.Remove(EquipmentChangedHandle);
	}
	BoundEquipmentManager.Reset();
	EquipmentChangedHandle.Reset();

	if (ULyraPawnComponent_CharacterParts* CharacterParts = BoundCharacterParts.Get())
	{
		CharacterParts->OnCharacterPartsChanged.RemoveDynamic(this, &ThisClass::HandleCharacterPartsChanged);
	}
	BoundCharacterParts.Reset();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged);
	}

#if WITH_EDITOR
	GetMutableDefault<UFpsFirstPersonSettings>()->OnSettingChanged().Remove(SettingsChangedHandle);
	SettingsChangedHandle.Reset();
#endif
}

void UFpsFirstPersonWeaponComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	RequestSync();
}

void UFpsFirstPersonWeaponComponent::HandleCharacterPartsChanged(ULyraPawnComponent_CharacterParts* ComponentWithChangedParts)
{
	RequestSync();
}

void UFpsFirstPersonWeaponComponent::HandleFirstPersonAnimInitialized()
{
	RequestSync();
}

void UFpsFirstPersonWeaponComponent::HandleWeaponEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	RequestSync();
}

void UFpsFirstPersonWeaponComponent::HandleEquipmentChanged(ULyraEquipmentManagerComponent* EquipmentManager)
{
	RequestSync();
}

void UFpsFirstPersonWeaponComponent::HandleWeaponTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	if (UpdatedComponent && UpdatedComponent->GetAttachParent())
	{
		RequestSync();
	}
}

#if WITH_EDITOR
void UFpsFirstPersonWeaponComponent::HandleSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent)
{
	RequestSync();
}
#endif

void UFpsFirstPersonWeaponComponent::UpdateFirstPersonLayers(USkeletalMeshComponent* FirstPersonMesh, const USkeletalMeshComponent* ThirdPersonBody)
{
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	if (!FirstPersonMesh)
	{
		DestroyWorldLayerLegs();
		return;
	}

	if (!Settings->UseFirstPersonRendering() || !ThirdPersonBody || !ThirdPersonBody->AnimClass.Get())
	{
		FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
		DestroyWorldLayerLegs();
		return;
	}

	if (FirstPersonMesh->FirstPersonPrimitiveType != EFirstPersonPrimitiveType::FirstPerson)
	{
		FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	}

	if (WorldLayerLegs && WorldLayerLegs->GetAttachParent() != FirstPersonMesh)
	{
		DestroyWorldLayerLegs();
	}

	if (!WorldLayerLegs)
	{
		// Not named "FirstPerson*": FindFirstPersonMesh matches on that.
		WorldLayerLegs = NewObject<USkeletalMeshComponent>(GetOwner(), TEXT("FpsWorldLayerLegs"), RF_Transient);
		WorldLayerLegs->SetupAttachment(FirstPersonMesh);
		WorldLayerLegs->SetOnlyOwnerSee(true);
		WorldLayerLegs->SetCastShadow(false);
		WorldLayerLegs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldLayerLegs->SetGenerateOverlapEvents(false);
		WorldLayerLegs->RegisterComponent();
	}

	if (WorldLayerLegs->GetSkeletalMeshAsset() != FirstPersonMesh->GetSkeletalMeshAsset())
	{
		WorldLayerLegs->SetSkeletalMesh(FirstPersonMesh->GetSkeletalMeshAsset());
		const TArray<UMaterialInterface*> Materials = FirstPersonMesh->GetMaterials();
		for (int32 Index = 0; Index < Materials.Num(); ++Index)
		{
			WorldLayerLegs->SetMaterial(Index, Materials[Index]);
		}
		// Copies the pose of its attach parent (the 1P body, after the arm IK).
		WorldLayerLegs->SetAnimInstanceClass(ThirdPersonBody->AnimClass.Get());
		for (const FName& BoneName : Settings->FirstPersonLayerBones)
		{
			if (WorldLayerLegs->GetBoneIndex(BoneName) != INDEX_NONE)
			{
				WorldLayerLegs->HideBoneByName(BoneName, PBO_None);
			}
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
	LocalWeaponSockets.Reset();
	LocalWeaponRelative.Reset();
	AppliedGripVersion = INDEX_NONE;
	AppliedGripVisual.Reset();
}

void UFpsFirstPersonWeaponComponent::ApplyWeaponGrip(AActor* LocalWeaponVisual)
{
	USkeletalMeshComponent* FirstPersonMesh = CachedFirstPersonMesh.IsValid() ? CachedFirstPersonMesh.Get() : FindFirstPersonMesh();
	UFpsFirstPersonAnimInstance* AnimInstance = FirstPersonMesh ? Cast<UFpsFirstPersonAnimInstance>(FirstPersonMesh->GetAnimInstance()) : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	const UFpsWeaponGripComponent* Grip = LocalWeaponVisual ? LocalWeaponVisual->FindComponentByClass<UFpsWeaponGripComponent>() : nullptr;
	AnimInstance->SetWeaponGrip(Grip ? Grip->GetGripProfile() : nullptr);

	FTransform WeaponInHand;
	int32 GripVersion = INDEX_NONE;
	if (!Grip || LocalWeaponSockets.IsEmpty() || !AnimInstance->GetLockedGrip(WeaponInHand, GripVersion))
	{
		AnimInstance->ConfirmLockedGripAttached(INDEX_NONE);
		return;
	}

	if (AppliedGripVersion != GripVersion || AppliedGripVisual.Get() != LocalWeaponVisual)
	{
		// The visual's socket sits on the weapon bone; express it in primary hand space.
		FTransform SocketInWeaponBone = FTransform::Identity;
		if (const USkeletalMeshSocket* Socket = FirstPersonMesh->GetSocketByName(LocalWeaponSockets[0]))
		{
			if (Socket->BoneName != AnimInstance->GetWeaponBone())
			{
				AnimInstance->ConfirmLockedGripAttached(INDEX_NONE);
				return;
			}
			SocketInWeaponBone = Socket->GetSocketLocalTransform();
		}
		else if (LocalWeaponSockets[0] != AnimInstance->GetWeaponBone())
		{
			AnimInstance->ConfirmLockedGripAttached(INDEX_NONE);
			return;
		}

		LocalWeaponVisual->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AnimInstance->GetPrimaryHandBone());
		LocalWeaponVisual->SetActorRelativeTransform(LocalWeaponRelative[0] * SocketInWeaponBone * WeaponInHand);
		AppliedGripVersion = GripVersion;
		AppliedGripVisual = LocalWeaponVisual;
	}

	AnimInstance->ConfirmLockedGripAttached(GripVersion);
}

void UFpsFirstPersonWeaponComponent::HideFromOwnerKeepShadow(UPrimitiveComponent* Primitive) const
{
	if (!Primitive)
	{
		return;
	}

	if (!bOwnerSeesThirdPersonShadow)
	{
		Primitive->SetOwnerNoSee(true);
		return;
	}

	if (GetDefault<UFpsFirstPersonSettings>()->UseFirstPersonRendering())
	{
		// Engine first person rendering: hidden from the owner, casts the owner's shadow.
		if (Primitive->FirstPersonPrimitiveType != EFirstPersonPrimitiveType::WorldSpaceRepresentation)
		{
			Primitive->SetRenderInMainPass(true);
			Primitive->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
		}
		return;
	}

	// OwnerNoSee also drops the owner's shadow of it (bCastHiddenShadow does not bring a skinned
	// mesh's shadow back), so instead keep it out of the main pass. This runs only on the owning
	// client for its own pawn; everyone else still sees the mesh normally.
	Primitive->SetOwnerNoSee(false);
	Primitive->SetRenderInMainPass(false);
}

USkeletalMeshComponent* UFpsFirstPersonWeaponComponent::ApplyOwnerBodyShadow(const USkeletalMeshComponent* FirstPersonMesh) const
{
	USkeletalMeshComponent* ThirdPersonBody = nullptr;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return nullptr;
	}

	// Third-person cosmetic body parts (owner-hidden) on the pawn's child actors.
	TArray<UChildActorComponent*> ChildActorComponents;
	Character->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (const UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		const AActor* PartActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (!PartActor)
		{
			continue;
		}

		TInlineComponentArray<USkeletalMeshComponent*> Meshes(PartActor);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh != FirstPersonMesh && Mesh->CastShadow
				&& (Mesh->bOwnerNoSee || !Mesh->bRenderInMainPass || Mesh->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::WorldSpaceRepresentation))
			{
				HideFromOwnerKeepShadow(Mesh);
				if (!ThirdPersonBody)
				{
					ThirdPersonBody = Mesh;
				}
			}
		}
	}

	return ThirdPersonBody;
}

void UFpsFirstPersonWeaponComponent::SyncFirstPersonWeaponVisuals()
{
	bSyncRequested = false;

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !HasBegunPlay())
	{
		return;
	}

	BindPawnSources(Pawn);
	if (!Pawn->IsLocallyControlled())
	{
		return;
	}

	ULyraEquipmentManagerComponent* EquipmentManager = BoundEquipmentManager.Get();
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

	WatchReplicatedWeapons(ReplicatedWeapons);

	USkeletalMeshComponent* FirstPersonMesh = FindFirstPersonMesh();
	CachedFirstPersonMesh = FirstPersonMesh;
	BindFirstPersonMesh(FirstPersonMesh);
	UpdateFirstPersonLayers(FirstPersonMesh, ApplyOwnerBodyShadow(FirstPersonMesh));

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
		ApplyWeaponGrip(nullptr);
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
					// Hidden from the owner without a shadow: its shadow would fall across the
					// view-locked 1P weapon right in front of the camera and read as a second gun.
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
					// No weapon shadow for the owner: this view-locked copy sits away from the body.
					Primitive->SetCastShadow(false);
					// Drawn with the arms in the first person layer: never sinks into walls or the legs.
					Primitive->SetFirstPersonPrimitiveType(GetDefault<UFpsFirstPersonSettings>()->UseFirstPersonRendering() ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None);
				}
			}

			LocalWeaponVisuals.Add(Visual);
			LocalWeaponSockets.Add(AttachSocket);
			LocalWeaponRelative.Add(RelativeXf);
		}
	}

	// Re-applied each sync: cosmetics can recreate the 1P anim instance, and a locked grip re-hangs the visual.
	ApplyWeaponGrip(LocalWeaponVisuals.Num() > 0 ? LocalWeaponVisuals[0].Get() : nullptr);
}
