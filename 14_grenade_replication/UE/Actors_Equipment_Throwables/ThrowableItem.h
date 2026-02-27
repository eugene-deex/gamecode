#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include "ThrowableItem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32);

USTRUCT(BlueprintType)
struct FThrowInfo
{
	GENERATED_BODY();

	FThrowInfo() :
		Loc_Mul_10(FVector::ZeroVector),
		Dir(FVector::ZeroVector)
	{}

	FThrowInfo(FVector Loc, FVector Dir) :
		Loc_Mul_10(Loc * 10.f),
		Dir(Dir)
	{}

	UPROPERTY()
		FVector_NetQuantize100 Loc_Mul_10;

	UPROPERTY()
		FVector_NetQuantizeNormal Dir;

	FVector GetLoc() const { return Loc_Mul_10 * 0.1f; }
	FVector GetDir() const { return Dir; }
};

UCLASS(Blueprintable)
class HOMEWORK1_API AThrowableItem : public AEquipableItem
{
	GENERATED_BODY()
	
public:
	AThrowableItem();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	FOnAmmoChanged OnAmmoChanged;
	
	//void Throw(FThrowInfo& Info);
	virtual void SetOwner(AActor* NewOwner) override;

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	void SetAmmo(int32 NewAmmo);
	FORCEINLINE bool CanThrow() const { return Ammo > 0; };
	FORCEINLINE int32 GetMaxAmmo() const { return MaxAmmo; };

	
	void Throw();

	FORCEINLINE ABaseCharacter* GetCharOwner() const { return CachedOwner.IsValid() ? CachedOwner.Get() : nullptr; };

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo", meta = (ClampMin = 1, UIMin = 1))
	int32 MaxAmmo = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables")
	TSubclassOf<class AGCProjectile> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throwables", meta = (UIMin = -90.f, UIMax = 90.f, ClampMin = -90.f, ClampMax = 90.f))
	float ThrowAngle = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables", meta = (UIMin = 1, ClampMin = 1))
	int32 ProjPoolSize = 10;

private:
	int32 Ammo = 0;

	UFUNCTION()
	void ProcessProjHit(AGCProjectile* Proj, const FHitResult& HitResult, const FVector& Dir);

	UPROPERTY(ReplicatedUsing = OnRep_LastGrenadeInfo);
	FThrowInfo LastInfo;

	UFUNCTION()
	void OnRep_LastGrenadeInfo();

	UFUNCTION(NetMulticast, Reliable)
	void ThrowInternal(const FThrowInfo& Info);

	// grenade pool
	UPROPERTY(Replicated)
	TArray<AGCProjectile*> GrenadePool;

	UPROPERTY(Replicated)
	int32 CurrentGrenadeIndex = 0;

	const FVector GrenadePoolLoc = FVector(-11910.f, -10076.f, 174.f);

	AController* GetCtrl() const;
	TWeakObjectPtr<ABaseCharacter> CachedOwner;

	UFUNCTION()
	void OnProjExplode(AActor* Proj);
};
