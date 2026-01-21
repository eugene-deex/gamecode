#include "Turret.h"
#include "Components/Weapon/WeaponBarrelComponent.h"
#include "Components/CharacterComponents/CharacterAttributesComponent.h"
#include "Characters/BaseCharacter.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Damage.h"

ATurret::ATurret()
{
 	PrimaryActorTick.bCanEverTick = true;
	
	USceneComponent* TurretRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TurretRoot"));
	SetRootComponent(TurretRoot);

	TurretBaseComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TurretBase"));
	TurretBaseComponent->SetupAttachment(TurretRoot);

	TurretBarrelComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TurretBarrel"));
	TurretBarrelComponent->SetupAttachment(TurretBaseComponent);

	WeaponBarrel = CreateDefaultSubobject<UWeaponBarrelComponent>(TEXT("WeaponBarrel"));
	WeaponBarrel->SetupAttachment(TurretBarrelComponent);

	Health = MaxHealth;
}

void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FMath::IsNearlyZero(Health))
		return;

	if (!IsTargetValid(CurrentTarget))
		CurrentTurretState = ETurretState::Searching;

	switch (CurrentTurretState)
	{
		case ETurretState::Searching:
		{
			SearchingMovement(DeltaTime);
			break;
		}

		case ETurretState::Firing:
		{
			FiringMovement(DeltaTime);
			break;
		}
	}

	// Draw turret health
	FVector TextLocation = GetRootComponent()->GetComponentLocation() + (30.f * FVector::UpVector);
	bool bDropShadow = true;
	float time = 0.f; // draw text in 1 frame

	FColor color = FColor::Green;
	if (Health < (MaxHealth / 3))
	{
		color = FColor::Red;
	}
	DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("Health: %.2f"), Health), nullptr, color, time, bDropShadow);
}

void ATurret::PossessedBy(AController* Ctrl)
{
	Super::PossessedBy(Ctrl);

	AAIController* AICtrl = Cast<AAIController>(Ctrl);
	if (IsValid(AICtrl))
	{
		FGenericTeamId TeamId((uint8)Team);
		AICtrl->SetGenericTeamId(TeamId);
	}
}

float ATurret::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount < 0.f || FMath::IsNearlyZero(DamageAmount) || Health < 0.f || FMath::IsNearlyZero(Health) || DamageCauser == nullptr)
		return 0.f;

	Health -= DamageAmount;
	if (Health < 0.f)
		Health = 0.f;

	if (FMath::IsNearlyZero(Health))
	{
		OnDie();
		return 0.f;
	}

	AActor* Damager = (EventInstigator && EventInstigator->GetPawn()) ? EventInstigator->GetPawn()  : DamageCauser;

	UAISense_Damage::ReportDamageEvent(
		GetWorld(),
		this, // DamagedActor
		Damager,
		DamageAmount,
		GetActorLocation(),			// EventLocation
		Damager->GetActorLocation() // InstigatorLocation
	);
	
	return DamageAmount;
}



bool ATurret::IsTargetValid(AActor* Target)
{
	if (!IsValid(Target))
		return false;

	if (ABaseCharacter* Char = Cast<ABaseCharacter>(Target))
		return Char->GetCharacterAttributesComponent()->IsAlive();
	else
		return false;
}

void ATurret::OnDie()
{
	GEngine->AddOnScreenDebugMessage(0, 2.f, FColor::Green, TEXT("Turrel is dead"));
	CurrentTurretState = ETurretState::Searching;
	GetWorld()->GetTimerManager().ClearTimer(ShotTimer);
	CurrentTarget = nullptr;

	if (IsValid(ExplosionVFX))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionVFX, GetRootComponent()->GetComponentLocation());
	}

	Destroy();
}

void ATurret::SetCurrentTarget(AActor* NewTarget)
{
	if (IsTargetValid(NewTarget))
	{
		CurrentTarget = NewTarget;
		SetCurrentTurretState(ETurretState::Firing);
	}
}

FVector ATurret::GetPawnViewLocation() const
{
	return WeaponBarrel->GetComponentLocation();
}

FRotator ATurret::GetViewRotation() const
{
	return WeaponBarrel->GetComponentRotation();
}

void ATurret::Stop()
{
	CurrentTarget = nullptr;
	SetCurrentTurretState(ETurretState::Searching);
}

void ATurret::SearchingMovement(float DeltaTime)
{
	FRotator TurretBaseRot = TurretBaseComponent->GetRelativeRotation();
	TurretBaseRot.Yaw += DeltaTime * BaseSearchingRotationRate;
	TurretBaseComponent->SetRelativeRotation(TurretBaseRot);

	FRotator TurrentBarrelRot = TurretBarrelComponent->GetRelativeRotation();
	TurrentBarrelRot.Pitch = FMath::FInterpTo(TurrentBarrelRot.Pitch, 0.f, DeltaTime, BarrelPitchRotationRate);
	TurretBarrelComponent->SetRelativeRotation(TurrentBarrelRot);
}

void ATurret::FiringMovement(float DeltaTime)
{
	// vector from turrel center to the current target
	FVector BaseLookAtDir = (CurrentTarget->GetActorLocation() - TurretBaseComponent->GetComponentLocation()).GetSafeNormal2D();
	FQuat LookAtQuat = BaseLookAtDir.ToOrientationQuat();
	FQuat TargetQuat = FMath::QInterpTo(TurretBaseComponent->GetComponentQuat(), LookAtQuat, DeltaTime, BaseFiringInterpSpeed);
	// rotate turrel to the target
	TurretBaseComponent->SetWorldRotation(TargetQuat);
	
	// set pitch angle
	FVector BarellLookAtDir = (CurrentTarget->GetActorLocation() - TurretBarrelComponent->GetComponentLocation()).GetSafeNormal();
	float PitchAngle = BarellLookAtDir.ToOrientationRotator().Pitch;

	FRotator CurrentRot = TurretBarrelComponent->GetRelativeRotation();
	CurrentRot.Pitch = FMath::FInterpTo(CurrentRot.Pitch, PitchAngle, DeltaTime, BarrelPitchRotationRate);

	TurretBarrelComponent->SetRelativeRotation(CurrentRot);
}

void ATurret::MakeShot()
{
	FVector ShotLoc = WeaponBarrel->GetComponentLocation();
	FVector ShotDir = WeaponBarrel->GetComponentRotation().RotateVector(FVector::ForwardVector);
	float SpreadAngle = FMath::DegreesToRadians(BulletSpreadAngle);

	WeaponBarrel->Shot(ShotLoc, ShotDir, SpreadAngle);
}

void ATurret::SetCurrentTurretState(ETurretState NewState)
{
	bool bIsStateChange = CurrentTurretState != NewState;
	CurrentTurretState = NewState;
	
	switch (CurrentTurretState)
	{
		case ETurretState::Searching:
		{
			GetWorld()->GetTimerManager().ClearTimer(ShotTimer);
			break;
		}

		case ETurretState::Firing:
		{
			GetWorld()->GetTimerManager().SetTimer(ShotTimer, this, &ATurret::MakeShot, GetFireInterval(), true, FireDelayTime);
			GetWorld()->GetTimerManager().ClearTimer(ShotTimerStop);
			GetWorld()->GetTimerManager().SetTimer(ShotTimerStop, this, &ATurret::Stop, FireStopOnLoseSight, false);
			break;
		}

	}
}
