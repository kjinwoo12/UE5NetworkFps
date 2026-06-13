// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FpsCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;

UCLASS()
class UE5NETWORKFPS_API AFpsCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFpsCharacter();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 1P view mesh (Only Owner See). Child of the 3P character mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	/** 1P camera. Child of FirstPersonMesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
};
