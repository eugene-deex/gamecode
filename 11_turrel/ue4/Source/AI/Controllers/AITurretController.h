#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIModule/Classes/Perception/AIPerceptionTypes.h"
#include "AITurretController.generated.h"

class ATurret;

UCLASS()
class HOMEWORK1_API AAITurretController : public AAIController
{
	GENERATED_BODY()
	
public:
	AAITurretController();
	virtual void SetPawn(APawn* InPawn) override;
	virtual void ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors) override;
	
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	TWeakObjectPtr<ATurret> CachedTurret;
};
