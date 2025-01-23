#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WeaponBarrelComponent.generated.h"

UENUM(BlueprintType)
enum class EHitRegistrationType : uint8
{
	HitScan,
	Projectile
};

class UNiagaraSystem;
class AGCProjectile;

USTRUCT(BlueprintType)
struct FDecalInfo
{
	GENERATED_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	UMaterialInterface* DecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	FVector DecalSize = FVector(5.f, 5.f, 5.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	float DecalLifetTime = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	float DecalFadeOutTime = 5.f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORK1_API UWeaponBarrelComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	void Shot(FVector ShotStart, FVector ShotDirection, float SpreadAngle);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes")
	float FiringRange = 5000.f; // rango de tiro

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes", meta=(ClampMin=1, UIMin=1))
	int32 BulletsPerShot = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | Damage")
	float DamageAmount = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | VFX")
	UNiagaraSystem* MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | VFX")
	UNiagaraSystem* TraceFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | Damage")
	UCurveFloat* FalloffDiagram;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | Damage")
	TSubclassOf<class UDamageType> DmgTypeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | Decals")
	FDecalInfo DefaultDecalInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | Hit Reg")
	EHitRegistrationType HitRegType = EHitRegistrationType::HitScan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel attributes | Hit Reg", meta = (EditCondition = "HitRegType == EHitRegistrationType::Projectile"))
	TSubclassOf<AGCProjectile> ProjectileClass;

private:
	FVector GetBulletSpreadOffset(float Angle, FRotator ShotRotation);
	bool HitScan(FVector ShotStart, FVector ShotDirection, OUT FVector& ShotEnd);
	void LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDir);
	APawn* GetOwningPawn() const;
	AController* GetCtrl() const;

	UFUNCTION()
	void ProcessHit(const FHitResult& HitResult, const FVector& Dir);
};
