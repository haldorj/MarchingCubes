// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MarchingCubes/MarchingChunk.h"

#include "MarchingCubes/Utility/GridMetrics.h"


APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	ThrustForce = 1000.0f;
	MaxFuel = 100.0f;
	MaxJetpackSpeed = 500.0f;
	CurrentFuel = MaxFuel;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APlayerCharacter::Jump()
{
	Super::Jump();
	bIsSpaceBarDown = true;
}

void APlayerCharacter::StopJumping()
{
	Super::StopJumping();
	bIsSpaceBarDown = false;
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);

	if (bIsSpaceBarDown && CurrentFuel > 0.0f)
	{
		ApplyThrust();
		CurrentFuel -= DeltaTime * 10;
	}
	if (CurrentFuel < 100.f)
		CurrentFuel += DeltaTime;

	
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Jetpack fuel: %i\n"), static_cast<int>(CurrentFuel)));	
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APlayerCharacter::Jump);
	PlayerInputComponent->BindAction("StopJumping", IE_Released, this, &APlayerCharacter::StopJumping); 

	PlayerInputComponent->BindAxis("Terraform", this, &APlayerCharacter::DeformMesh);
	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APlayerCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerCharacter::LookUp);

}

void APlayerCharacter::ApplyThrust()
{
	GetMovementComponent()->Velocity.Z = MaxJetpackSpeed;
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}

void APlayerCharacter::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		
		float DistanceToCharacter = (this->GetActorLocation() - Start).Size();
		Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		
		if (!TraceHitResult.bBlockingHit) TraceHitResult.ImpactPoint = End;
		else
		{
			DrawDebugSphere(
			GetWorld(),
			TraceHitResult.ImpactPoint,
			BrushSize * 50,
			12,
			FColor::Green
			);
			DrawDebugPoint(
			GetWorld(),
			TraceHitResult.ImpactPoint,
			16,
			FColor::Green
			);
		}
		TraceHitInfo = TraceHitResult;
	}
}

void APlayerCharacter::Terraform(float Value)
{
	EditWeights(Value);
}

void APlayerCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void APlayerCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void APlayerCharacter::EditWeights(float terraform)
{
	if (TraceHitInfo.GetActor() && TraceHitInfo.GetActor()->IsA<AMarchingChunk>())
	{
		AMarchingChunk* Chunk = Cast<AMarchingChunk>(TraceHitInfo.GetActor());
		if (Chunk)
		{
			int ChunkSize = Chunk->GridMetrics.PointsPerChunk;
			FVector HitPositionLocal = Chunk->GetTransform().InverseTransformPosition(TraceHitInfo.ImpactPoint);
			// Calculate the influence area based on the brush size
			float BrushRadiusSq = BrushSize * BrushSize;
			float BrushRadiusSqInverse = 1.0f / BrushRadiusSq;

			for (int x = 0; x < ChunkSize; x++)
			{
				for (int y = 0; y < ChunkSize; y++)
				{
					for (int z = 0; z < ChunkSize; z++)
					{
						// Check whether we are inside of our grid
						if (x >= ChunkSize - 1 || y >= ChunkSize - 1 || z >= ChunkSize - 1)
						{
							return;
						}
						// Calculate influence based on distance
						float DistSq = (FVector(x, y, z) * Chunk->GridMetrics.Distance - HitPositionLocal).SizeSquared();
						float Influence = FMath::Clamp(1.0f - (DistSq * BrushRadiusSqInverse), 0.0f, 1.0f);

						if (DistSq < BrushRadiusSq)
						{
							// Modify the weight value instead of directly modifying the vertices
							Chunk->Weights[Chunk->IndexFromCoord(x, y, z)] += terraform * Influence * TerraformStrength;
						}
					}
				}
			}

			// Update the procedural mesh with the modified weights
			Chunk->UpdateMesh();
		}
	}
}

void APlayerCharacter::DeformMesh(float terraform)
{
	// Check if the hit result has a valid actor and if it's the terrain you want to deform
	if (TraceHitInfo.GetActor() && TraceHitInfo.GetActor()->IsA<AMarchingChunk>())
	{
		// Cast the hit actor to the terrain type (AMarchingChunk)
		AMarchingChunk* Chunk = Cast<AMarchingChunk>(TraceHitInfo.GetActor());
		if (Chunk)
		{
			if(GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Seed: %i\n"), Chunk->Seed));			
			// Get the hit position in local space of the terrain
			FVector HitPositionLocal = Chunk->GetTransform().InverseTransformPosition(TraceHitInfo.ImpactPoint);

			// Calculate the influence area based on the brush size
			float BrushRadiusSq = BrushSize * BrushSize;
			float BrushRadiusSqInverse = 1.0f / BrushRadiusSq;

			// Perform terrain deformation within the influence area
			for (int32 i = 0; i < Chunk->Verts.Num(); i++)
			{
				FVector VertexLocal = Chunk->Verts[i];
				float DistSq = (VertexLocal - HitPositionLocal).SizeSquared();
				if (DistSq < BrushRadiusSq)
				{
					// Calculate influence based on distance
					float Influence = FMath::Clamp(1.0f - (DistSq * BrushRadiusSqInverse), 0.0f, 1.0f);

					// Apply the deformation to the vertex
					FVector n = TraceHitInfo.Normal.GetSafeNormal() * terraform * Influence * TerraformStrength;
					VertexLocal += n;
					
					// Update the vertex in the chunk's vertices array
					Chunk->Verts[i] = VertexLocal;
				}
			}

			// Update the procedural mesh with the modified vertices
			Chunk->UpdateMesh();
		}
	}
}