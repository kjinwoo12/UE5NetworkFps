// Copyright Epic Games, Inc. All Rights Reserved.

#include "FpsWeaponGripComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FpsWeaponGripComponent)

UFpsWeaponGripComponent::UFpsWeaponGripComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}
