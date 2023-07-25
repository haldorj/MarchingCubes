// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

#define TRACE_LENGTH 80000.f

UCLASS()
class MARCHINGCUBES_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:
	virtual void BeginPlay() override;

	void Jump() override;
	void StopJumping() override;
	
	void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
	void LookUp(float Value);

	void ApplyThrust();
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
private:
	bool Terraform;
	
	UPROPERTY(EditAnywhere, Category = "Terraforming")
	float BrushSize = 2.0f;
	
	UPROPERTY(EditAnywhere, Category = "Jetpack")
	float ThrustForce;

	UPROPERTY(EditAnywhere, Category = "Jetpack")
	float MaxFuel;
	
	UPROPERTY(EditAnywhere, Category = "Jetpack")
	float MaxJetpackSpeed;
	
	float CurrentFuel;
	bool bIsSpaceBarDown;
	
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, Category=Camera)
	class UCameraComponent* FollowCamera;
public:	

};
