#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharAttrsWidget.generated.h"

UCLASS()
class HOMEWORK1_API UCharAttrsWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Attributes")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Attributes")
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Attributes")
	float Oxygen;

	UFUNCTION(BlueprintCallable)
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable)
	float GetStaminaPercent() const;
	
	UFUNCTION(BlueprintCallable)
	float GetOxygenPercent() const;

private:
	
	UFUNCTION()
	void UpdateHealth(float NewHealth) { Health = NewHealth; };
	
	UFUNCTION()
	void UpdateStamina(float NewStamina) { Stamina = NewStamina; };
	
	UFUNCTION()
	void UpdateOxygen(float NewOxygen) { Oxygen = NewOxygen; };
};
