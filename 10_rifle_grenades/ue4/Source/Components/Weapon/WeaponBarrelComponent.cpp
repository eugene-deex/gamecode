#include "WeaponBarrelComponent.h"
#include "HomeWorkTypes.h"
#include "DrawDebugHelpers.h"
#include "Subsystems/DebugSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/DecalComponent.h"
#include "Actors/Projectiles/GCProjectile.h"

void UWeaponBarrelComponent::Shot(FVector ShotStart, FVector ShotDirection, float SpreadAngle)
{
#if ENABLE_DEBUG_DRAW
	bool bIsDebug = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>()->bIsCategoryEnabled(DebugCategoryRangeWeapon);
#else 
	bool bIsDebug = false;
#endif

	if (MuzzleFlashFX == nullptr || !MuzzleFlashFX->IsValid())
	{
		UE_LOG(LogDamage, Warning, TEXT("UWeaponBarrelComponent::Shot MuzzleFlashFX is not set"));
		return;
	}

	if (TraceFX == nullptr || !TraceFX->IsValid())
	{
		UE_LOG(LogDamage, Warning, TEXT("UWeaponBarrelComponent::Shot TraceFX is not set"));
		return;
	}

	FVector MuzzleLocation = GetComponentLocation();
	// Muzzle Flash FX
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation, GetComponentRotation());

	for (int i = 0; i < BulletsPerShot; ++i)
	{
		FVector CustomShotDir = ShotDirection + GetBulletSpreadOffset(FMath::RandRange(0.f, SpreadAngle), ShotDirection.ToOrientationRotator());
		FVector ShotEnd = ShotStart + FiringRange * CustomShotDir;

		UE_LOG(LogTemp, Warning, TEXT("UWeaponBarrelComponent::Shot %s"), *CustomShotDir.ToCompactString());

		switch (HitRegType)
		{
		case EHitRegistrationType::HitScan:
		{
			bool bHasHit = HitScan(ShotStart, CustomShotDir, ShotEnd);
			if (bIsDebug && bHasHit) DrawDebugSphere(GetWorld(), ShotEnd, 10.f, 24, FColor::Red, false, 1.f);
			break;
		}
		case EHitRegistrationType::Projectile:
		{
			LaunchProjectile(ShotStart, CustomShotDir);
			break;
		}
		}

		// Trace smoke FX
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TraceFX, MuzzleLocation, GetComponentRotation());
		NiagaraComp->SetVectorParameter(FXParamTraceEnd, ShotEnd);

		if (bIsDebug) DrawDebugLine(GetWorld(), MuzzleLocation, ShotEnd, FColor::Red, false, 1.f, 0, 3.f);
	}
}

FVector UWeaponBarrelComponent::GetBulletSpreadOffset(float Angle, FRotator ShotRotation)
{
	float SpreadSize = FMath::Tan(Angle);
	float RotationAngle = FMath::RandRange(0.f, 2.f * PI); // 0 .. 360

	float SpreadY = FMath::Cos(RotationAngle);
	float SpreadZ = FMath::Sin(RotationAngle);

	FVector Result = (ShotRotation.RotateVector(FVector::UpVector) * SpreadZ
		+ ShotRotation.RotateVector(FVector::RightVector) * SpreadY) * SpreadSize;
	return Result;
}

bool UWeaponBarrelComponent::HitScan(FVector ShotStart, FVector ShotDirection, OUT FVector& ShotEnd)
{
	FHitResult ShotResult;
	
	if (!GetWorld()->LineTraceSingleByChannel(ShotResult, ShotStart, ShotEnd, ECC_Bullet))
	{
		return false;
	}

	ShotEnd = ShotResult.ImpactPoint;
	ProcessHit(ShotResult, ShotDirection);

	return true;
}

void UWeaponBarrelComponent::LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDir)
{
	AGCProjectile* Proj = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, LaunchStart, LaunchDir.ToOrientationRotator());
	if (IsValid(Proj))
	{		
		Proj->SetOwner(GetOwningPawn());
		Proj->OnProjectileHit.AddDynamic(this, &UWeaponBarrelComponent::ProcessHit);
		Proj->LaunchProjectile(LaunchDir.GetSafeNormal());
	}
}

// can be cached only if update it on Owner Pawn change - weapon can be used by other controllers
APawn* UWeaponBarrelComponent::GetOwningPawn() const
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!IsValid(PawnOwner))
	{
		PawnOwner = Cast<APawn>(GetOwner()->GetOwner());
	}

	return PawnOwner;
}

AController* UWeaponBarrelComponent::GetCtrl() const
{
	APawn* PawnOwner = GetOwningPawn();
	return IsValid(PawnOwner) ? PawnOwner->GetController() : nullptr; 
}

void UWeaponBarrelComponent::ProcessHit(const FHitResult& HitResult, const FVector& Dir)
{
	AActor* HitActor = HitResult.GetActor();
	if (IsValid(HitActor))
	{
		float Damage = DamageAmount;
		if (IsValid(FalloffDiagram))
		{
			float Distance = FVector::Distance(GetOwningPawn()->GetActorLocation(), HitResult.ImpactPoint); // TODO: count dist from current player position
			Damage = FalloffDiagram->GetFloatValue(Distance) * DamageAmount;
			UE_LOG(LogDamage, Warning, TEXT("UWeaponBarrelComponent::Shot() Distance: %f, Damage: %f"), Distance, Damage);
		}

		FPointDamageEvent DmgEvent;
		DmgEvent.HitInfo = HitResult;
		DmgEvent.ShotDirection = Dir;
		DmgEvent.DamageTypeClass = DmgTypeClass;
		HitActor->TakeDamage(Damage, DmgEvent, GetCtrl(), GetOwner()); // Shot !!!
	}

	UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),
		DefaultDecalInfo.DecalMaterial,
		DefaultDecalInfo.DecalSize,
		HitResult.ImpactPoint,
		HitResult.ImpactNormal.ToOrientationRotator()
	);

	if (IsValid(DecalComp))
	{
		DecalComp->SetFadeScreenSize(0.001f);
		DecalComp->SetFadeOut(DefaultDecalInfo.DecalLifetTime, DefaultDecalInfo.DecalFadeOutTime);
	}
}
