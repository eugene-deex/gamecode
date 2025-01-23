#include "CharacterAttributesComponent.h"
#include "Characters/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Subsystems/DebugSubsystem.h"
#include "HomeWorkTypes.h"

// Sets default values for this component's properties
UCharacterAttributesComponent::UCharacterAttributesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UCharacterAttributesComponent::BeginPlay()
{
	Super::BeginPlay();
	
	checkf(MaxHealth > 0.0f, TEXT("UCharacterAttributesComponent::BeginPlay max health cannot be equal to 0"));
	checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("UCharacterAttributesComponent::BeginPlay can be used only with ABaseCharacter"))

	CachedChar = StaticCast<ABaseCharacter*>(GetOwner());
	
	Health = MaxHealth;
	CurrentStamina = MaxStamina;
	Oxygen = MaxOxygen;

	CachedChar->OnTakeAnyDamage.AddDynamic(this, &UCharacterAttributesComponent::OnTakeAnyDamage);

	if (OnHealthChangedEvent.IsBound())
	{
		OnHealthChangedEvent.Broadcast((Health / MaxHealth) * 100.f);
	}

	if (OnStaminaChangedEvent.IsBound())
	{
		OnStaminaChangedEvent.Broadcast((CurrentStamina / MaxStamina) * 100.f);
	}
	
	if (OnOxygenChangedEvent.IsBound())
	{
		OnOxygenChangedEvent.Broadcast((Oxygen / MaxOxygen) * 100.f);
	}
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void UCharacterAttributesComponent::DebugDrawAttributes()
{

	if (!IsAlive())
	{
		return;
	}

	UDebugSubsystem* DebugSubSystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
	if (!DebugSubSystem->bIsCategoryEnabled(DebugCategoryCharacterAttributes))
	{
		return;
	}
	
	// Health
	FVector TextLocation = CachedChar->GetActorLocation() + (CachedChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f) * FVector::UpVector;
	float time = 0.f; // draw text in 1 frame
	bool bDropShadow = true;

	FColor color = FColor::Green;
	if (Health < (MaxHealth / 3))
	{
		color = FColor::Red;
	}

	DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("Health: %.2f"), Health), nullptr, color, time, bDropShadow);

	// Stamina
	FVector StaminaTextLocation = TextLocation + FVector(0, 0, -10);
	DrawDebugString(GetWorld(), StaminaTextLocation, FString::Printf(TEXT("Stamina: %.2f"), CurrentStamina), nullptr, FColor::Blue, 0.f, true);

	// Oxygen
	FVector OxygenTextLocation = TextLocation + FVector(0, 0, -20);
	DrawDebugString(GetWorld(), OxygenTextLocation, FString::Printf(TEXT("Oxygen: %.2f"), Oxygen), nullptr, FColor::Cyan, 0.f, true);
}
#endif

void UCharacterAttributesComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return;
	}

	UE_LOG(LogDamage, Warning, TEXT("UCharacterAttributesComponent::OnTakeAnyDamage %s received %.2f amount of damage from %s"), *CachedChar->GetName(), Damage, *DamageCauser->GetName());
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	if (OnHealthChangedEvent.IsBound())
	{
		OnHealthChangedEvent.Broadcast((Health / MaxHealth) * 100.f);
	}

	if (Health <= 0.f)
	{
		bool bIsSwimming = CachedChar->GetMoveComp()->IsSwimming();
		UE_LOG(LogDamage, Warning, TEXT("UCharacterAttributesComponent::OnTakeAnyDamage char %s is killed by an actor %s; is swimming: %s"), 
			*CachedChar->GetName(), *DamageCauser->GetName(), bIsSwimming ? TEXT("TRUE") : TEXT("FALSE"));
		if (OnDeathEvent.IsBound())
		{
			OnDeathEvent.Broadcast(bIsSwimming);
		}
	}
}

// Called every frame
void UCharacterAttributesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateStaminaValue(DeltaTime);
	UpdateOxygenValue(DeltaTime);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	DebugDrawAttributes();
#endif
}

void UCharacterAttributesComponent::UpdateOxygenValue(float DeltaTime)
{
	if (CachedChar->IsSwimmingUnderWater())
	{
		Oxygen -= SwimOxygenConsumptionVelocity * DeltaTime;
	} else {
		Oxygen += OxygenRestoreVelocity * DeltaTime;
	}

	Oxygen = FMath::Clamp(Oxygen, 0.f, MaxOxygen);

	if (OnOxygenChangedEvent.IsBound())
	{
		OnOxygenChangedEvent.Broadcast((Oxygen / MaxOxygen) * 100.f);
	}

	if (Oxygen <= 0.f)
	{
		TimePassed += DeltaTime;
		if (TimePassed >= NoOxygenDamageFrequency)
		{
			CachedChar->TakeDamage(NoOxygenDamageAmount, FDamageEvent(), GetOwner()->GetInstigatorController(), GetOwner());
			TimePassed = 0.f;
			UE_LOG(LogDamage, Warning, TEXT("UCharacterAttributesComponent::UpdateOxygenValue damage %.f2 is applied"), NoOxygenDamageAmount);
		}
	}
}

void UCharacterAttributesComponent::UpdateStaminaValue(float DeltaTime)
{	
	// consumpt/restore stamina
	if (CachedChar->GetMoveComp()->IsSprinting())
	{
		CurrentStamina -= StaminaConsumptionSpeed * DeltaTime;

		if (CurrentStamina < 0.f)
		{
			if (OutOfStaminaEvent.IsBound())
			{
				OutOfStaminaEvent.Broadcast(true);
			}
		}

		CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);
	}
	else {
		CurrentStamina += StaminaRestoreSpeed * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);

		if (CurrentStamina >= MaxStamina)
		{
			if (OutOfStaminaEvent.IsBound())
			{
				OutOfStaminaEvent.Broadcast(false);
			}
		}
	}

	if (OnStaminaChangedEvent.IsBound())
	{
		OnStaminaChangedEvent.Broadcast((CurrentStamina / MaxStamina) * 100.f);
	}
}

