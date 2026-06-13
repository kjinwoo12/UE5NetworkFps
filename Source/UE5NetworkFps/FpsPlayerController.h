// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FpsPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class UE5NETWORKFPS_API AFpsPlayerController : public APlayerController
{
	GENERATED_BODY()
	
private:
	UPROPERTY(EditAnywhere,  category = "Input", meta = (bAllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere,  category = "Input", meta = (bAllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> LookMappingContext;

public:
	void BeginPlay() override;
};
