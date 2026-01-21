#include "DeexCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "HomeWork1/Components/Movement/BaseMovementComponent.h"
#include "HomeWork1/Components/CharacterComponents/CharacterAttributesComponent.h"
#include "HomeWork1/Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"

#include "Kismet/KismetSystemLibrary.h"
#include "HomeWork1/HomeWork1.h"
#include "Controllers/BasePlayerController.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"

void ADeexCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AimingTimeline.TickTimeline(DeltaSeconds);
}

ADeexCharacter::ADeexCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = 1;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // Z rotation speed 540

	BaseMovementComponent = StaticCast<UBaseMovementComponent*>(GetCharacterMovement());

	Team = ETeams::Player;
} 

void ADeexCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ADeexCharacter::MoveForward(float Value)
{
	if ((GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()) && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		// turn Player Mesh after Mouse (controller) turned
		FRotator YawRotator(0.f, GetControlRotation().Yaw, 0.f); // Y Z X
		FVector ForwardVector = YawRotator.RotateVector(FVector::ForwardVector); // X (-1..1) Y(-1..1)  Z(0)
		AddMovementInput(ForwardVector, Value);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDebug)
		{
			GEngine->AddOnScreenDebugMessage(0, 2.f, FColor::Green, FString::Printf(TEXT("ControlRotation Yaw: %f | %f %f %f"), GetControlRotation().Yaw, ForwardVector.X, ForwardVector.Y, ForwardVector.Z));
		}
#endif
	}
}

void ADeexCharacter::MoveRight(float Value)
{
	if ((GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()) && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator YawRotator(0.f, GetControlRotation().Yaw, 0.f); // Y Z X
		FVector RightVector = YawRotator.RotateVector(FVector::RightVector); // X (-1..1) Y(-1..1)  Z(0)
		AddMovementInput(RightVector, Value);
	}
}

void ADeexCharacter::Turn(float Value)
{
	AddControllerYawInput(Value * GetAimingMod());
}

void ADeexCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value * GetAimingMod());
}

void ADeexCharacter::TurnAtRate(float Value)
{	
	AddControllerYawInput(Value * GetAimingMod() * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ADeexCharacter::LookUpAtRate(float Value)
{
	AddControllerPitchInput(Value  * GetAimingMod() * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ADeexCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SpringArmComponent->TargetOffset += FVector(0.f, 0.f, HalfHeightAdjust);
}

void ADeexCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SpringArmComponent->TargetOffset += FVector(0.f, 0.f, -HalfHeightAdjust);
}

void ADeexCharacter::SwimForward(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		// turn Player Mesh after Mouse (controller) turned
		FRotator PitchYawRotator(GetControlRotation().Pitch, GetControlRotation().Yaw, 0.f); // Y Z X     - here we move player in direction of mouse look (up or down) using Pitch
		FVector ForwardVector = PitchYawRotator.RotateVector(FVector::ForwardVector); // X (-1..1) Y(-1..1)  Z(0)
		AddMovementInput(ForwardVector, Value);
	}
}

void ADeexCharacter::SwimRight(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator YawRotator(0.f, GetControlRotation().Yaw, 0.f); // Y Z X
		FVector RightVector = YawRotator.RotateVector(FVector::RightVector); // X (-1..1) Y(-1..1)  Z(0)
		AddMovementInput(RightVector, Value);
	}
}

void ADeexCharacter::SwimUp(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		AddMovementInput(FVector::UpVector, Value);
	}
}

// Hard landing
void ADeexCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!CharacterAttributesComponent->IsAlive())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstance) || !HardLandingMontage)
	{
		UE_LOG(LogDeex, Error, TEXT("ADeexCharacter::Landed no montage set"));
		return;
	}

	float CurrentZ = GetActorLocation().Z;

	if (CurrentZ < (topReachedZ - HardlandingMinHeight))
	{
		UE_LOG(LogDeex, Warning, TEXT("current %f | reached %f | DO HARD LAND!"), CurrentZ, topReachedZ);
		float Duration = AnimInstance->Montage_Play(HardLandingMontage, 1.f, EMontagePlayReturnType::Duration, 0.f);

		if (PlayerController.IsValid())
		{
			PlayerController->SetIgnoreLookInput(true); // to fix the head when FP player is mantling up
			PlayerController->SetIgnoreMoveInput(true);
			GetWorld()->GetTimerManager().SetTimer(MontageTimer, this, &ADeexCharacter::UnlockInputBlock, Duration, false);
		}
	}
}

void ADeexCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PlayerController = Cast<ABasePlayerController>(Controller);
}

void ADeexCharacter::OnStartAimingInternal()
{
	Super::OnStartAimingInternal();

	APlayerController* PController = GetController<APlayerController>();
	if (!IsValid(PController))
	{
		return;
	}

	APlayerCameraManager* CameraManager = PController->PlayerCameraManager;
	if (IsValid(CameraManager))
	{
		ARangeWeaponItem* RangeWeapon = GetEquipmentComp()->GetCurrentRangeWeapon();
		if (IsValid(AimingDiagram))
		{
			AimingTimeline.SetPlayRate(1.f);
			FOnTimelineFloatStatic AimingUpdate;
			AimingUpdate.BindUObject(this, &ADeexCharacter::AimingFOVUpdate);
			AimingTimeline.AddInterpFloat(AimingDiagram, AimingUpdate);
			AimingTimeline.Play();
		}else if(IsValid(RangeWeapon)){
			CameraManager->SetFOV(RangeWeapon->GetAimFOV());
		}
	}
}

void ADeexCharacter::AimingFOVUpdate(const float Alpha)
{
	APlayerController* PController = GetController<APlayerController>();
	if (!IsValid(PController))
	{
		return;
	}

	APlayerCameraManager* CameraManager = PController->PlayerCameraManager;
	if (IsValid(CameraManager))
	{
		ARangeWeaponItem* RangeWeapon = GetEquipmentComp()->GetCurrentRangeWeapon();
		if (IsValid(RangeWeapon))
		{
			CameraManager->SetFOV(FMath::Lerp(CameraManager->DefaultFOV, RangeWeapon->GetAimFOV(), Alpha));
		}
	}
}

void ADeexCharacter::OnStopAimingInternal()
{
	Super::OnStopAimingInternal();

	APlayerController* PController = GetController<APlayerController>();
	if (!IsValid(PController))
	{
		return;
	}

	APlayerCameraManager* CameraManager = PController->PlayerCameraManager;
	if (IsValid(CameraManager))
	{
		if (IsValid(AimingDiagram))
		{
			AimingTimeline.Reverse();
		} else {
			CameraManager->UnlockFOV();
		}	
	}
}

float ADeexCharacter::GetAimingMod() const
{
	ARangeWeaponItem* RangeWeapon = GetEquipmentComp()->GetCurrentRangeWeapon();
	return IsValid(RangeWeapon) ? RangeWeapon->GetAimTurnModifier() : 1.f;
}

void ADeexCharacter::UnlockInputBlock()
{
	if (PlayerController.IsValid())
	{
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetIgnoreMoveInput(false);
	}
}
