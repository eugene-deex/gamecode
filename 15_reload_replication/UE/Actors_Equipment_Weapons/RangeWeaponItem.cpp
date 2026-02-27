#include "RangeWeaponItem.h"
#include "Components/Weapon/WeaponBarrelComponent.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "HomeWorkTypes.h"
#include "Characters/BaseCharacter.h"
#include "AI/Controllers/BaseAIController.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

ARangeWeaponItem::ARangeWeaponItem()
{
	SetReplicates(true);
	//NetUpdateFrequency = 100.f;   // no effect
	//MinNetUpdateFrequency = 100.f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	WeaponBarrel = CreateDefaultSubobject<UWeaponBarrelComponent>(TEXT("WeaponBarrel"));
	WeaponBarrel->SetupAttachment(WeaponMesh, SocketWeaponMuzzle);

	WeaponBarrelAlt = CreateDefaultSubobject<UWeaponBarrelComponent>(TEXT("WeaponBarrelAlt"));
	WeaponBarrelAlt->SetupAttachment(WeaponMesh, SocketWeaponMuzzle);
	WeaponBarrelAlt->bIsAlt = true;

	EquippedSocketName = SocketCharacterWeapon;
	ReticleType = EReticleType::Default;
}

void ARangeWeaponItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARangeWeaponItem, Ammo);
	DOREPLIFETIME(ARangeWeaponItem, Ammo2);
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
	Srv_StartReload();
}
void ARangeWeaponItem::Srv_StartReload_Implementation()
{
	StartReloadInternal();
}

bool ARangeWeaponItem::StartReloadInternal_Validate()
{
	return !bIsReloading; // cancel duplicate reloading RPC
}

void ARangeWeaponItem::StartReloadInternal_Implementation()
{
	if (bIsReloading)
		return;
	
	bIsReloading = true;

	ABaseCharacter* BaseChar = GetCharOwner();
	if (!IsValid(BaseChar))
	{
		return;
	}

	const UCharacterEquipmentComponent* EquipComp = BaseChar->GetEquipmentComp();
	if (EquipComp->GetAvailableAmmoForCurWeapon() == 0)
	{
		return;
	}

	if (IsValid(CharacterReloadMontage))
	{
		float MontageDuration = BaseChar->PlayAnimMontage(CharacterReloadMontage);
		PlayAnimMontage(WeaponReloadMontage);
		if (ReloadType == EReloadType::FullClip)
		{
			GetWorld()->GetTimerManager().SetTimer(ReloadTimer, [this]() { ARangeWeaponItem::EndReload(true); }, MontageDuration, false);
		}
	}
	else {
		EndReload(true);
	}
}

void ARangeWeaponItem::EndReload(bool bIsSuccess)
{
	if (!bIsReloading)
	{
		return;
	}

	ABaseCharacter* BaseChar = GetCharOwner();

	if (!bIsSuccess)
	{
		if (IsValid(BaseChar))
		{
			BaseChar->StopAnimMontage(CharacterReloadMontage);
		}
		StopAnimMontage(WeaponReloadMontage, false);
	}

	if (ReloadType == EReloadType::ByBullet)
	{		
		UAnimInstance* CharAnimIns = IsValid(BaseChar) ? BaseChar->GetMesh()->GetAnimInstance() : nullptr;
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
	ABaseCharacter* CharacterOwner = GetCharOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}
	
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
	
	FVector ShotLoc = FVector::ZeroVector;
	FRotator ShotRot = FRotator::ZeroRotator;

	if (CharacterOwner->IsPlayerControlled())
	{
		APlayerController* Controller = CharacterOwner->GetController<APlayerController>();
		if (!IsValid(Controller)) 
			return;
		Controller->GetPlayerViewPoint(ShotLoc, ShotRot);
	} else {
		ShotLoc = WeaponBarrel->GetComponentLocation();
		ShotRot = CharacterOwner->GetBaseAimRotation();
	}

	FVector ShotDir = ShotRot.RotateVector(FVector::ForwardVector);
	
	SetAmmo(FMath::Max(GetAmmo() - 1, 0));

	GetBarrelComp()->Shot(ShotLoc, ShotDir, GetCurrentBulletSpreadAngle());

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
