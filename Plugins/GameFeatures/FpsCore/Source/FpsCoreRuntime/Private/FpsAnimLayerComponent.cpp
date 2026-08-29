// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsAnimLayerComponent.h"

#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Cosmetics/LyraPawnComponent_CharacterParts.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "Weapons/LyraWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsAnimLayerComponent)

namespace FpsAnimLayerComponent_Private
{
	static TSubclassOf<UAnimInstance> LoadAnimLayerClass(const TCHAR* AssetPath)
	{
		return TSubclassOf<UAnimInstance>(FSoftClassPath(AssetPath).TryLoadClass<UAnimInstance>());
	}

	static TSubclassOf<UAnimInstance> SelectBestLayer(
		const FLyraAnimLayerSelectionSet& Layers,
		const FGameplayTagContainer& CosmeticTags)
	{
		for (const FLyraAnimLayerSelectionEntry& Rule : Layers.LayerRules)
		{
			if ((Rule.Layer != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
			{
				return Rule.Layer;
			}
		}
		return Layers.DefaultLayer;
	}

	static FGameplayTagContainer CollectCosmeticTags(const AActor* OwnerActor)
	{
		FGameplayTagContainer Result;
		if (!OwnerActor)
		{
			return Result;
		}

		TArray<UChildActorComponent*> ChildActorComponents;
		OwnerActor->GetComponents<UChildActorComponent>(ChildActorComponents);
		for (const UChildActorComponent* ChildActorComponent : ChildActorComponents)
		{
			if (!ChildActorComponent)
			{
				continue;
			}

			if (IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(ChildActorComponent->GetChildActor()))
			{
				TagInterface->GetOwnedGameplayTags(/*inout*/ Result);
			}
		}

		return Result;
	}

	static void AddTaggedLayer(
		FLyraAnimLayerSelectionSet& OutLayers,
		const TCHAR* LayerPath,
		const FGameplayTag& AnimStyleTag,
		const FGameplayTag& BodyStyleTag)
	{
		if (TSubclassOf<UAnimInstance> Layer = LoadAnimLayerClass(LayerPath))
		{
			FLyraAnimLayerSelectionEntry& Entry = OutLayers.LayerRules.AddDefaulted_GetRef();
			Entry.Layer = Layer;
			if (AnimStyleTag.IsValid())
			{
				Entry.RequiredTags.AddTag(AnimStyleTag);
			}
			if (BodyStyleTag.IsValid())
			{
				Entry.RequiredTags.AddTag(BodyStyleTag);
			}
		}
	}

	static void ConfigureThirdPersonUnarmed(FLyraAnimLayerSelectionSet& OutLayers)
	{
		OutLayers.LayerRules.Reset();

		const FGameplayTag MasculineTag = FGameplayTag::RequestGameplayTag(TEXT("Cosmetic.AnimationStyle.Masculine"), false);
		const FGameplayTag FeminineTag = FGameplayTag::RequestGameplayTag(TEXT("Cosmetic.AnimationStyle.Feminine"), false);
		const FGameplayTag MediumBodyTag = FGameplayTag::RequestGameplayTag(TEXT("Cosmetic.BodyStyle.Medium"), false);

		AddTaggedLayer(OutLayers,
			TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers.ABP_UnarmedAnimLayers_C"),
			MasculineTag, MediumBodyTag);
		AddTaggedLayer(OutLayers,
			TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers_Feminine.ABP_UnarmedAnimLayers_Feminine_C"),
			FeminineTag, MediumBodyTag);

		OutLayers.DefaultLayer = LoadAnimLayerClass(
			TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers.ABP_UnarmedAnimLayers_C"));
	}
}

UFpsAnimLayerComponent::UFpsAnimLayerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	FpsAnimLayerComponent_Private::ConfigureThirdPersonUnarmed(ThirdPersonUnarmedLayers);
}

void UFpsAnimLayerComponent::BeginPlay()
{
	Super::BeginPlay();

	BindToCosmeticsComponents();
	SyncCosmeticPartOwners();
	ScheduleLinkDefaultAnimLayers();
}

void UFpsAnimLayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredLinkTimerHandle);
	}

	for (const TWeakObjectPtr<ULyraPawnComponent_CharacterParts>& WeakPartsComponent : BoundCosmeticsComponents)
	{
		if (ULyraPawnComponent_CharacterParts* PartsComponent = WeakPartsComponent.Get())
		{
			PartsComponent->OnCharacterPartsChanged.RemoveDynamic(this, &ThisClass::HandleCharacterPartsChanged);
		}
	}

	BoundCosmeticsComponents.Reset();

	Super::EndPlay(EndPlayReason);
}

void UFpsAnimLayerComponent::BindToCosmeticsComponents()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	TArray<ULyraPawnComponent_CharacterParts*> PartsComponents;
	OwnerActor->GetComponents<ULyraPawnComponent_CharacterParts>(PartsComponents);

	for (ULyraPawnComponent_CharacterParts* PartsComponent : PartsComponents)
	{
		if (!PartsComponent)
		{
			continue;
		}

		const bool bAlreadyBound = BoundCosmeticsComponents.ContainsByPredicate(
			[PartsComponent](const TWeakObjectPtr<ULyraPawnComponent_CharacterParts>& Entry)
			{
				return Entry.Get() == PartsComponent;
			});

		if (bAlreadyBound)
		{
			continue;
		}

		PartsComponent->OnCharacterPartsChanged.AddDynamic(this, &ThisClass::HandleCharacterPartsChanged);
		BoundCosmeticsComponents.Add(PartsComponent);
	}
}

void UFpsAnimLayerComponent::HandleCharacterPartsChanged(ULyraPawnComponent_CharacterParts* /*PartsComponent*/)
{
	// ChildActorComponent.bSetOwner defaults false, so B_Manny has no Owner.
	// OwnerNoSee / OnlyOwnerSee test ViewTarget (pawn) against the owner chain.
	SyncCosmeticPartOwners();

	// SetSkeletalMesh(bReinitPose) runs in BroadcastChanged before this delegate;
	// defer one tick so the new AnimInstance exists before we re-link layers.
	ScheduleLinkDefaultAnimLayers();
}

void UFpsAnimLayerComponent::SyncCosmeticPartOwners()
{
	AActor* PawnOwner = GetOwner();
	if (!PawnOwner)
	{
		return;
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	PawnOwner->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		AActor* PartActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (PartActor && PartActor->GetOwner() != PawnOwner)
		{
			PartActor->SetOwner(PawnOwner);
		}
	}
}

void UFpsAnimLayerComponent::ScheduleLinkDefaultAnimLayers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		LinkDefaultAnimLayers();
		return;
	}

	World->GetTimerManager().ClearTimer(DeferredLinkTimerHandle);
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::LinkDefaultAnimLayers));
}

void UFpsAnimLayerComponent::RefreshAnimLayers()
{
	ScheduleLinkDefaultAnimLayers();
}

void UFpsAnimLayerComponent::LinkLayersOnMesh(USkeletalMeshComponent* Mesh, TSubclassOf<UAnimInstance> LayerClass)
{
	if (Mesh && LayerClass)
	{
		Mesh->LinkAnimClassLayers(LayerClass);
	}
}

void UFpsAnimLayerComponent::LinkDefaultAnimLayers()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	const FGameplayTagContainer CosmeticTags = FpsAnimLayerComponent_Private::CollectCosmeticTags(Character);

	// After CharacterParts SetSkeletalMesh(bReinitPose), linked layers are wiped.
	// If a weapon is equipped, re-apply its layers (do not early-out — that left Fps
	// loadout heroes with mesh but no rifle hold pose when BP GetTypedPawn failed
	// or cosmetics finished after OnEquipped).
	if (ULyraEquipmentManagerComponent* EquipmentManager = Character->FindComponentByClass<ULyraEquipmentManagerComponent>())
	{
		if (ULyraWeaponInstance* Weapon = EquipmentManager->GetFirstInstanceOfType<ULyraWeaponInstance>())
		{
			LinkLayersOnMesh(Character->GetMesh(), Weapon->PickBestAnimLayer(/*bEquipped=*/true, CosmeticTags));
			return;
		}
	}

	// Only CharacterMesh0 evaluates Base + linked Unarmed layers.
	// B_Manny MeshComponent = CopyPose, FirstPersonMesh = FirstPersonCopy — both copy this pose.
	LinkLayersOnMesh(
		Character->GetMesh(),
		FpsAnimLayerComponent_Private::SelectBestLayer(ThirdPersonUnarmedLayers, CosmeticTags));
}
