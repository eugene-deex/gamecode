#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCProjectile.generated.h"

class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileHit, const FHitResult&, Hit, const FVector&, Dir);

UCLASS()
class HOMEWORK1_API AGCProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	AGCProjectile();

	UFUNCTION(BlueprintCallable)
	void LaunchProjectile(FVector Dir);

	UPROPERTY(BlueprintAssignable)
	FOnProjectileHit OnProjectileHit;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	class USphereComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	class UProjectileMovementComponent* MoveComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Component")
	float Velocity = 2000.f;

	virtual void OnProjectileLaunched();
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
