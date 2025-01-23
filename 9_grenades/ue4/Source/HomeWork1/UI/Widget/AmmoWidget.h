#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AmmoWidget.generated.h"

UCLASS()
class HOMEWORK1_API UAmmoWidget : public UUserWidget
{ 
	GENERATED_BODY() 
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 Ammo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 TotalAmmo;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 Grenades;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	int32 MaxGrenades;

private:
	UFUNCTION()
	void UpdateAmmoCounter(int32 NewAmmo, int32 NewTotalAmmo);
	
	UFUNCTION()
	void UpdateGrenadesAmmoCounter(int32 NewGrenades, int32 MaxAmmo);
};