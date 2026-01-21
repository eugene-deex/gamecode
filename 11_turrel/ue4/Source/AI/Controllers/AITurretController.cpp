#include "AITurretController.h"
#include "AI/Characters/Turret.h"
#include "AIModule/Classes/AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AIModule/Classes/Perception/AIPerceptionTypes.h"
#include "AIModule/Classes/Perception/AISense_Damage.h"

AAITurretController::AAITurretController()
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("TurretPerception"));
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AAITurretController::OnPerceptionUpdated);
}

void AAITurretController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{ 
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			CachedTurret->SetCurrentTarget(Actor);
			UE_LOG(LogTemp, Log, TEXT("AAITurretController::OnPerceptionUpdated: Damage sensed from %s"), *GetNameSafe(Actor));
		}
	}
}

void AAITurretController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (IsValid(InPawn))
	{
		checkf(InPawn->IsA<ATurret>(), TEXT("AAITurretController::SetPawn AAITurretController can possess only Turrets"));
		CachedTurret = StaticCast<ATurret*>(InPawn);

	} else {
		CachedTurret = nullptr;
	}
}

// it receives all actors, including the ones that already left the scene
void AAITurretController::ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	if (!CachedTurret.IsValid())
		return;

	// get actors that have been seen recently
	TArray<AActor*> SeenActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SeenActors);
	
	AActor* ClosestActor = nullptr;
	float MinSquaredDistance = FLT_MAX;

	FVector TurretLoc = CachedTurret->GetActorLocation();

	for (AActor* SeenActor : SeenActors)
	{
		float DistSq = (TurretLoc - SeenActor->GetActorLocation()).SizeSquared();
		if (DistSq < MinSquaredDistance)
		{
			MinSquaredDistance = DistSq;
			ClosestActor = SeenActor;
		}
	}

	CachedTurret->SetCurrentTarget(ClosestActor);
}
