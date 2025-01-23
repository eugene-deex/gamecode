#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"

#include "RangeWeaponItem.generated.h"

class UAnimMontage;

DECLARE_MULTICAST_DELEGATE(FOnReloadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32);

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Single,
	FullAuto
};

UENUM(BlueprintType)
enum class EReloadType : uint8
{
	FullClip, // 1-time reload; adds ammo only when the whole reload animation is successfully played
	ByBullet // shotgun-like; requires character reload anim has 'ReloadEnd' section
};

UCLASS(Blueprintable)
class HOMEWORK1_API ARangeWeaponItem : public AEquipableItem
{
	GENERATED_BODY()

public:
	ARangeWeaponItem();

	ARangeWeaponItem* GetCurrentRangeWeapon() const;

	void StartFire();
	void StopFire();

	void StartReload();
	void EndReload(bool bIsSuccess);

	void StartAim() { bIsAiming = true; };
	void StopAim() { bIsAiming = false; };

	float GetAimFOV() const { return AimFOV; };
	float GetAimMovementMaxSpeed()  const { return AimMovementMaxSpeed; };
	float GetAimTurnModifier()  const { return AimTurnModifier; };
	float GetAimLookUpModifier()  const { return AimLookUpModifier; };

	FTransform GetForeGripTransform() const;

	FORCEINLINE int32 GetAmmo() const { return bIsAltMode ? Ammo2 : Ammo; }
	void SetAmmo(int32 NewAmmo);
	FORCEINLINE bool CanShoot() const { return (bIsAltMode ? Ammo2 : Ammo) > 0; };

	FORCEINLINE int32 GetMaxAmmo() const { return bIsAltMode ? MaxAmmo2 : MaxAmmo; };

	FOnAmmoChanged OnAmmoChanged;
	FOnReloadComplete OnReloadComplete;

	FORCEINLINE EAmunitionType GetAmmoType() const { return bIsAltMode ? AmmoType2 : AmmoType; };

	virtual EReticleType GetReticleType() const;
	void ToggleAltMode();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWeaponBarrelComponent* WeaponBarrel;
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWeaponBarrelComponent* WeaponBarrelAlt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	bool AltFireIsActive = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	UAnimMontage* WeaponFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	UAnimMontage* WeaponReloadMontage;
	
	// FullClip reload type adds ammo only when the whole reload animation is successfully played
	// ByBullet reload type requires character reload anim has 'ReloadEnd' section
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	EReloadType ReloadType = EReloadType::FullClip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Character")
	UAnimMontage* CharacterFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Character")
	UAnimMontage* CharacterReloadMontage;
	
	// rate of fire - rounds per minute
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters", meta = (ClampMin = 1.f, UIMin = 1.f))
	float FireRate = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters")
	EWeaponFireMode WeaponFireMode = EWeaponFireMode::Single;

	// Bullet spread half angle in degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters", meta = (ClampMin = 0.f, UIMin = 0.f, ClampMax = 2.f, UIMax = 2.f))
	float SpreadAngle = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.f, UIMin = 0.f, ClampMax = 2.f, UIMax = 2.f))
	float AimSpreadAngle = .25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.f, UIMin = 0.f))
	float AimMovementMaxSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.f, UIMin = 0.f, ClampMax = 120.f, UIMax = 120.f))
	float AimFOV = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.f, UIMin = 0.f, ClampMax = 1.f, UIMax = 1.f))
	float AimTurnModifier = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.f, UIMin = 0.f, ClampMax = 1.f, UIMax = 1.f))
	float AimLookUpModifier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo", meta = (ClampMin = 1, UIMin = 1))
	int32 MaxAmmo = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo", meta = (ClampMin = 1, UIMin = 1))
	EAmunitionType AmmoType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo Alt", meta = (ClampMin = 1, UIMin = 1))
	int32 MaxAmmo2 = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo Alt", meta = (ClampMin = 1, UIMin = 1))
	EAmunitionType AmmoType2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo")
	bool bAutoReload = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reticle")
	EReticleType AimReticleType = EReticleType::Default;

private:
	int32 Ammo = 0;
	int32 Ammo2 = 0;
	
	bool bIsReloading = false;
	bool bIsFiring = false;
	bool bIsAiming = false;
	bool bIsAltMode = false;

	float GetCurrentBulletSpreadAngle() const;
	void MakeShot();
	float GetShotTimeInterval() const;
	float PlayAnimMontage(UAnimMontage* AnimMontage); // helper
	void StopAnimMontage(UAnimMontage* Montage, float BlendOutTime = 0.f);
	void OnShotTimerElapsed();
	FORCEINLINE UWeaponBarrelComponent* GetBarrelComp() { return AltFireIsActive && bIsAltMode ? WeaponBarrelAlt : WeaponBarrel; };

	FTimerHandle ShotTimerHandle;
	FTimerHandle ReloadTimer;
};
