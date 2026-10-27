// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsFirstPersonSettings.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsFirstPersonSettings)

UFpsFirstPersonSettings::UFpsFirstPersonSettings()
{
	SectionName = TEXT("Fps First Person");
}

bool UFpsFirstPersonSettings::GetTrueFirstPersonEyeLocation(const APawn* Pawn, const FRotator& ViewRotation, FVector& OutLocation)
{
	const UFpsFirstPersonSettings* Settings = GetDefault<UFpsFirstPersonSettings>();
	if (!Settings->bTrueFirstPerson)
	{
		return false;
	}

	// The pawn's own mesh runs the full third-person animation; cosmetic meshes copy its pose.
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh || Mesh->GetBoneIndex(Settings->EyeBone) == INDEX_NONE)
	{
		return false;
	}

	OutLocation = Mesh->GetSocketLocation(Settings->EyeBone) + ViewRotation.RotateVector(Settings->EyeViewOffset);
	return true;
}
