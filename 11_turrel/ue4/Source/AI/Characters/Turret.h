#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HomeWorkTypes.h"
#include "Particles/ParticleSystem.h"
#include "Turret.generated.h"

UENUM(BlueprintType)
enum class ETurretState : uint8
{
	Searching,
	Firing
};

class UWeaponBarrelComponent;

UCLASS()
class HOMEWORK1_API ATurret : public APawn
{
	GENERATED_BODY()

public:
	ATurret();
	virtual void Tick(float DeltaTime) override;
	void SetCurrentTarget(AActor* NewTarget);

	virtual FVector GetPawnViewLocation() const override;
	virtual FRotator GetViewRotation() const override;

	virtual void PossessedBy(AController* Ctrl) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* TurretBaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* TurretBarrelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWeaponBarrelComponent* WeaponBarrel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
		float BaseSearchingRotationRate = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters",
		meta = (ClampMin = 0.f, UIMin = 0.f))
		float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float BarrelPitchRotationRate = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float BaseFiringInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float MaxBarrellPitchAngle = 60.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float MinBarrellPitchAngle = -30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire", 
		meta = (ClampMin = 1.f, UIMin = 0.f))
	float RateOfFire = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float BulletSpreadAngle = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire", 
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float FireDelayTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire",
		meta = (ClampMin = 0.f, UIMin = 0.f))
	float FireStopOnLoseSight = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Team")
	ETeams Team = ETeams::Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | VFX")
	UParticleSystem* ExplosionVFX;

	void Stop(); 
private:
	float Health;
	void SearchingMovement(float DeltaTime);
	void FiringMovement(float DeltaTime);
	float GetFireInterval() const { return 60.f / RateOfFire; };
	void MakeShot();
	bool IsTargetValid(AActor* Target);
	void OnDie();
	void SetCurrentTurretState(ETurretState NewState);
	ETurretState CurrentTurretState = ETurretState::Searching;
	FTimerHandle ShotTimer;
	FTimerHandle ShotTimerStop;
	AActor* CurrentTarget = nullptr;

};
