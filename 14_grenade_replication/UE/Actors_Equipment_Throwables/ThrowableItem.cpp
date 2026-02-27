#include "ThrowableItem.h"
#include "HomeworkTypes.h"
#include "Characters/BaseCharacter.h"
#include "Actors/Projectiles/GCProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Components/Weapon/WeaponBarrelComponent.h"
#include "Actors/Projectiles/ExplosiveProjectile.h"
#include "Utils/DeexUtils.h"
#include "Components/ExplosionComponent.h"

AThrowableItem::AThrowableItem()
{
	SetReplicates(true);
}

void AThrowableItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.Condition = COND_SimulatedOnly;
	RepParams.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS(AThrowableItem, LastInfo, RepParams);
	DOREPLIFETIME(AThrowableItem, GrenadePool);
	DOREPLIFETIME(AThrowableItem, CurrentGrenadeIndex);
}

void AThrowableItem::SetOwner(AActor* NewOwner)
{
	Super::SetOwner(NewOwner);

	if (IsValid(NewOwner))
	{
		checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("AEquipableItem::SetOwner() only BaseChar can be an owner of equipable item"));
		CachedOwner = StaticCast<ABaseCharacter*>(NewOwner);

		if (GetLocalRole() == ROLE_Authority)
		{
			SetAutonomousProxy(true);
		}
	}
	else {
		CachedOwner = nullptr;
	}
}

void AThrowableItem::BeginPlay()
{
	Super::BeginPlay();
	SetAmmo(MaxAmmo);

	// Make grenades pool
	GrenadePool.Reserve(ProjPoolSize);

	for (int32 i = 0; i < ProjPoolSize; ++i)
	{
		AGCProjectile* Proj = GetWorld()->SpawnActor<AGCProjectile>(
			GrenadeClass,
			GrenadePoolLoc + FVector(0, 0, 10 * i), FRotator::ZeroRotator);
		
		if (!Proj) {
			UE_LOG(LogTemp, Error, TEXT("Grenade Spawn failed"));
			return;
		}

		Proj->SetOwner(GetOwner());
		Proj->SetProjActive(false);

		GrenadePool.Add(Proj);
	}
}

void AThrowableItem::OnRep_LastGrenadeInfo()
{
	//ThrowInternal(LastInfo);
}

void AThrowableItem::Throw()
{
	ABaseCharacter* Char = GetCharOwner();
	if (!IsValid(Char))
	{
		return;
	}

	FVector ThrowLoc = FVector::ZeroVector;
	FRotator ThrowRot = FRotator::ZeroRotator;

	if (Char->IsPlayerControlled())
	{
		APlayerController* Ctrl = Char->GetController<APlayerController>();
		if (!IsValid(Ctrl)) return;

		Ctrl->GetPlayerViewPoint(ThrowLoc, ThrowRot);

	} else {

		ThrowLoc = Char->GetActorLocation();
		ThrowRot = Char->GetBaseAimRotation();
	}

	FVector ViewDir = ThrowRot.RotateVector(FVector::ForwardVector);
	FVector ViewUpVector = ThrowRot.RotateVector(FVector::UpVector);
	FVector LaunchDir = ViewDir + ViewUpVector * FMath::Tan(FMath::RadiansToDegrees(ThrowAngle)); // throw direction taking into account throw angle

	FVector ThrowSocketLoc = Char->GetMesh()->GetSocketLocation(SocketCharacterThrowable); // global coords
	FTransform ViewTransform(ThrowRot, ThrowLoc);
	FVector ViewSpaceSockCoords = ViewTransform.InverseTransformPosition(ThrowSocketLoc); // convert to user space
	FVector SpawnLoc = ThrowLoc + ViewDir * ViewSpaceSockCoords.X;

	const FThrowInfo Info(SpawnLoc, LaunchDir);
	
	LastInfo = Info;

	//UE_LOG(LogDeex, Warning, TEXT("Throw local role: %s"),
	//	DeexUtils::LOG_ENUM(GetLocalRole()) // Role
	//);

	ThrowInternal(Info);
}

void AThrowableItem::ThrowInternal_Implementation(const FThrowInfo& Info)
{
	//UE_LOG(LogDeex, Warning, TEXT("ThrowInternal_Implementation local role: %s"),
	//	DeexUtils::LOG_ENUM(GetLocalRole()) // Role
	//);

	if (GetCharOwner()->HasAuthority())
		LastInfo = Info;

	if (!CanThrow())
		return;

	if (!GrenadePool.Num())
	{
		UE_LOG(LogDeex, Warning, TEXT("Grenade pool is not ready yet")); // TODO: need to be fixed
		return;
	}

	AGCProjectile* Proj = GrenadePool[CurrentGrenadeIndex];

	if (IsValid(Proj) && !Proj->IsProjActive())
	{
		Proj->SetActorLocation(Info.GetLoc());
		Proj->SetActorRotation(Info.GetDir().ToOrientationRotator());
		Proj->SetProjActive(true);		
		Proj->SetOwner(GetOwner());
		
		AExplosiveProjectile* Expl = StaticCast<AExplosiveProjectile*>(Proj);
		if (Expl)
		{	
			UExplosionComponent* ExplComp = Expl->GetExplComp();

			if (!ExplComp->OnExplosion.Contains(this, FName("OnProjExplode")))
				ExplComp->OnExplosion.AddDynamic(this, &AThrowableItem::OnProjExplode);

		} else {
			if (!Proj->OnProjectileHit.Contains(this, FName("ProcessProjHit")))
				Proj->OnProjectileHit.AddDynamic(this, &AThrowableItem::ProcessProjHit);
		}

		SetAmmo(FMath::Max(Ammo - 1, 0));
		Proj->LaunchProjectile(Info.GetDir().GetSafeNormal());
	}

	++CurrentGrenadeIndex;
	if (CurrentGrenadeIndex == GrenadePool.Num())
		CurrentGrenadeIndex = 0;
}

void AThrowableItem::OnProjExplode(AActor* Proj)
{
	AExplosiveProjectile* Expl = StaticCast<AExplosiveProjectile*>(Proj);

	if (Expl)
	{
		Expl->SetActorLocation(GrenadePoolLoc);
		Expl->SetProjActive(false);
		Expl->SetActorRotation(FRotator::ZeroRotator);
		Expl->OnProjectileHit.RemoveAll(this);

		UExplosionComponent* ExplComp = Expl->GetExplComp();
		ExplComp->OnExplosion.RemoveAll(this);
	}
}

void AThrowableItem::ProcessProjHit(AGCProjectile* Proj, const FHitResult& HitResult, const FVector& Dir)
{
	//DeexUtils::Sphere(GetWorld(), Proj->GetActorLocation(), true, FColor::Red); // hit point
	Proj->SetActorLocation(GrenadePoolLoc);
	Proj->SetProjActive(false);
	Proj->SetActorRotation(FRotator::ZeroRotator);
	Proj->OnProjectileHit.RemoveAll(this);
}

void AThrowableItem::SetAmmo(int32 NewAmmo)
{
	Ammo = FMath::Clamp(NewAmmo, 0, MaxAmmo);

	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast(NewAmmo);
	}
}