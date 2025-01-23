#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterAttributesComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeathEventSignature, bool)
DECLARE_MULTICAST_DELEGATE_OneParam(FOutOfStaminaEventSignature, bool)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthChangedEvent, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaminaChangedEvent, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOxygenChangedEvent, float);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORK1_API UCharacterAttributesComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharacterAttributesComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FOnDeathEventSignature OnDeathEvent;
	FOutOfStaminaEventSignature OutOfStaminaEvent;

	FOnHealthChangedEvent OnHealthChangedEvent;
	FOnStaminaChangedEvent OnStaminaChangedEvent;
	FOnOxygenChangedEvent OnOxygenChangedEvent;

	bool IsAlive() { return Health > 0.f; }

	FORCEINLINE float GetHealthPercent() const { return (Health / MaxHealth) * 100.f; }

	void UpdateStaminaValue(float DeltaTime);
	void UpdateOxygenValue(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Stamina", meta = (ClampMin = 0.f, UIMin = 0.f))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Stamina", meta = (ClampMin = 0.f, UIMin = 0.f))
	float StaminaRestoreSpeed = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Stamina", meta = (ClampMin = 0.f, UIMin = 0.f))
	float StaminaConsumptionSpeed = 60.f;

	// used in WBP_DeexHUD Get_StaminaBar_Percent_0 function to update Stamina bar
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetCurrentStamina() const { return CurrentStamina; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Oxygen", meta = (ClampMin = 0.f))
	float MaxOxygen = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Oxygen", meta = (ClampMin = 0.f))
	float OxygenRestoreVelocity = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Oxygen", meta = (ClampMin = 0.f))
	float SwimOxygenConsumptionVelocity = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Oxygen", meta = (ClampMin = 0.f))
	float NoOxygenDamageAmount = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Oxygen", meta = (ClampMin = 0.f))
	float NoOxygenDamageFrequency = 2.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta = (UIMin = 0.f))
	float MaxHealth = 100.f;

private:
	float Health = 0.f;
	float Oxygen;  // current amount
	float TimePassed = 0.f; // to make no-oxy damage every N sec.

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void DebugDrawAttributes();
#endif

	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	TWeakObjectPtr<class ABaseCharacter> CachedChar;

	float CurrentStamina;
};
