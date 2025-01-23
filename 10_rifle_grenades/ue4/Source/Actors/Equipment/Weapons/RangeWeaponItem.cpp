#include "RangeWeaponItem.h"
#include "Components/Weapon/WeaponBarrelComponent.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "HomeWorkTypes.h"
#include "Characters/BaseCharacter.h"

ARangeWeaponItem::ARangeWeaponItem()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	WeaponBarrel = CreateDefaultSubobject<UWeaponBarrelComponent>(TEXT("WeaponBarrel"));
	WeaponBarrel->SetupAttachment(WeaponMesh, SocketWeaponMuzzle);

	WeaponBarrelAlt = CreateDefaultSubobject<UWeaponBarrelComponent>(TEXT("WeaponBarrelAlt"));
	WeaponBarrelAlt->SetupAttachment(WeaponMesh, SocketWeaponMuzzle);

	EquippedSocketName = SocketCharacterWeapon;
	ReticleType = EReticleType::Default;
}

void ARangeWeaponItem::StartFire()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(ShotTimerHandle))
	{
		return;
	}

	bIsFiring = true;
	MakeShot();
}

void ARangeWeaponItem::StopFire()
{
	bIsFiring = false;

	if ((bIsAltMode ? AmmoType2 : AmmoType) == EAmunitionType::RifleGrenades && GetMaxAmmo() > 0)
	{
		StartReload();
	}
}

void ARangeWeaponItem::StartReload()
{
	checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("ARangeWeaponItem::StartReload() only BaseChar can be an owner of range weapon"));
	ABaseCharacter* BaseChar = StaticCast<ABaseCharacter*>(GetOwner());

	const UCharacterEquipmentComponent* EquipComp = BaseChar->GetEquipmentComp();
	if (EquipComp->GetAvailableAmmoForCurWeapon() == 0)
	{
		return;
	}

	bIsReloading = true;
	if (IsValid(CharacterReloadMontage))
	{
		float MontageDuration = BaseChar->PlayAnimMontage(CharacterReloadMontage);
		PlayAnimMontage(WeaponReloadMontage);
		if (ReloadType == EReloadType::FullClip)
		{
			GetWorld()->GetTimerManager().SetTimer(ReloadTimer, [this]() { ARangeWeaponItem::EndReload(true); }, MontageDuration, false);
		}
	} else {
		EndReload(true);
	}
}

void ARangeWeaponItem::EndReload(bool bIsSuccess)
{
	if (!bIsReloading)
	{
		return;
	}

	if (!bIsSuccess)
	{
		checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("ARangeWeaponItem::endReload() only BaseChar can be an owner of range weapon"));
		ABaseCharacter* BaseChar = StaticCast<ABaseCharacter*>(GetOwner());
		BaseChar->StopAnimMontage(CharacterReloadMontage);
		StopAnimMontage(WeaponReloadMontage, false);
	}

	if (ReloadType == EReloadType::ByBullet)
	{
		ABaseCharacter* BaseChar = StaticCast<ABaseCharacter*>(GetOwner());
		UAnimInstance* CharAnimIns = BaseChar->GetMesh()->GetAnimInstance();
		if (IsValid(CharAnimIns))
		{
			CharAnimIns->Montage_JumpToSection(SectionMontageReloadEnd, CharacterReloadMontage);
		}

		UAnimInstance* WeapAnimInstance = WeaponMesh->GetAnimInstance();
		if (IsValid(WeapAnimInstance))
		{
			WeapAnimInstance->Montage_JumpToSection(SectionMontageReloadEnd, WeaponReloadMontage);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);

	bIsReloading = false;
	if (bIsSuccess && OnReloadComplete.IsBound())
	{
		OnReloadComplete.Broadcast();
	}
}

FTransform ARangeWeaponItem::GetForeGripTransform() const
{
	return WeaponMesh->GetSocketTransform(SocketWeaponForeGrip);
}

void ARangeWeaponItem::SetAmmo(int32 NewAmmo)
{
	(bIsAltMode ? Ammo2 : Ammo) = FMath::Clamp(NewAmmo, 0, GetMaxAmmo());

	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast((bIsAltMode ? Ammo2 : Ammo));
	}
}

EReticleType ARangeWeaponItem::GetReticleType() const
{
	return bIsAiming ? AimReticleType : ReticleType;
}

void ARangeWeaponItem::ToggleAltMode()
{
	if (AltFireIsActive) bIsAltMode = !bIsAltMode;
	//(AltFireIsActive? Ammo2 : Ammo) = 0;
	if (!CanShoot()) StartReload();

	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast((bIsAltMode ? Ammo2 : Ammo));
	}
}

void ARangeWeaponItem::BeginPlay()
{
	Super::BeginPlay();
	SetAmmo(GetMaxAmmo()); // AmmunitionArray[(uint32)GetAmmoType()]
}

float ARangeWeaponItem::GetCurrentBulletSpreadAngle() const
{
	float AngleDegrees = bIsAiming ? AimSpreadAngle : SpreadAngle;
	return FMath::DegreesToRadians(AngleDegrees);
}

void ARangeWeaponItem::MakeShot()
{
	checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("ARangeWeaponItem::Fire() only BaseChar can be an owner of a range weapon"));
	ABaseCharacter* CharacterOwner = StaticCast<ABaseCharacter*>(GetOwner());

	if (!CanShoot())
	{
		if (bAutoReload && GetAmmo() == 0)
		{
			CharacterOwner->Reload();
		} else {
			StopFire();
		}
		return;
	}

	EndReload(false);

	if (IsValid(CharacterFireMontage)) CharacterOwner->PlayAnimMontage(CharacterFireMontage);
	if (IsValid(WeaponFireMontage)) PlayAnimMontage(WeaponFireMontage);
	
	APlayerController* Controller = CharacterOwner->GetController<APlayerController>();
	if (!IsValid(Controller)) return; // e.g. controller is AI

	FVector Loc = FVector::ZeroVector;
	FRotator PlayerViewRotation = FRotator::ZeroRotator;
	Controller->GetPlayerViewPoint(Loc, PlayerViewRotation);

	FVector PlayerViewDirection = PlayerViewRotation.RotateVector(FVector::ForwardVector);
	
	SetAmmo(FMath::Max(GetAmmo() - 1, 0));

	GetBarrelComp()->Shot(Loc, PlayerViewDirection, GetCurrentBulletSpreadAngle());

	GetWorld()->GetTimerManager().SetTimer(ShotTimerHandle, this, &ARangeWeaponItem::OnShotTimerElapsed, GetShotTimeInterval(), false);
}

float ARangeWeaponItem::GetShotTimeInterval() const
{
	return 60.f / FireRate;
}

float ARangeWeaponItem::PlayAnimMontage(UAnimMontage* AnimMontage)
{
	UAnimInstance* WeaponAnimInst = WeaponMesh->GetAnimInstance();
	return IsValid(WeaponAnimInst) ? WeaponAnimInst->Montage_Play(AnimMontage) : 0.f;
}

void ARangeWeaponItem::StopAnimMontage(UAnimMontage* Montage, float BlendOutTime /*= 0.f*/)
{
	UAnimInstance* WeaponAnimInst = WeaponMesh->GetAnimInstance();
	if (IsValid(WeaponAnimInst))
	{
		WeaponAnimInst->Montage_Stop(BlendOutTime, Montage);
	}
}

void ARangeWeaponItem::OnShotTimerElapsed()
{
	if (!bIsFiring)
	{
		return;
	}

	switch (WeaponFireMode)
	{
		case EWeaponFireMode::Single:
			StopFire();
			break;
		case EWeaponFireMode::FullAuto:
			MakeShot(); // make 1 shot more
			break;
	}
}
