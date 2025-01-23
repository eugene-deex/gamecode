#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include "ThrowableItem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32);

UCLASS(Blueprintable)
class HOMEWORK1_API AThrowableItem : public AEquipableItem
{
	GENERATED_BODY()
	
public:
	AThrowableItem();

	FOnAmmoChanged OnAmmoChanged;
	void Throw();
	
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	void SetAmmo(int32 NewAmmo);
	FORCEINLINE bool CanThrow() const { return Ammo > 0; };
	FORCEINLINE int32 GetMaxAmmo() const { return MaxAmmo; };

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo", meta = (ClampMin = 1, UIMin = 1))
	int32 MaxAmmo = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables")
	TSubclassOf<class AGCProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throwables", meta = (UIMin = -90.f, UIMax = 90.f, ClampMin = -90.f, ClampMax = 90.f))
	float ThrowAngle = 0.f;

private:
	int32 Ammo = 0;
};
