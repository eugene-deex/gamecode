#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_Grenade.generated.h"

UCLASS()
class HOMEWORK1_API UBTService_Grenade : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_Grenade();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	FBlackboardKeySelector IsThrowing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float MinFireDistance = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float MaxFireDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float CooldownTime = 5.f;

private:
	FTimerHandle CooldownTimer;
	void ResetCooldown();
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwner;
};
