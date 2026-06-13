// Fill out your copyright notice in the Description page of Project Settings.

#include "FpsPlayerController.h"

void AFpsPlayerController::BeginPlay() 
{
  Super::BeginPlay();

  if (IsLocalPlayerController()) 
  {
    UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (Subsystem)
    {
      if(DefaultMappingContext)
      {
        Subsystem->AddMappingContext(DefaultMappingContext, 0);
      }

      if(LookMappingContext)
      {
        Subsystem->AddMappingContext(LookMappingContext, 0);
      }
    }
  }
}