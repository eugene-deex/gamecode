#include "AIPatrollingComponent.h"
#include "Actors/Nav/PatrollingPath.h"

bool UAIPatrollingComponent::CanPatrol() const
{
	return IsValid(PatrollingPath) && PatrollingPath->GetWayPoints().Num() > 0;
}

FVector UAIPatrollingComponent::SelectClosestWayPoint()
{
	FVector OwnerLocation = GetOwner()->GetActorLocation();
	const TArray<FVector> Wayps = PatrollingPath->GetWayPoints();
	FTransform PathTransform = PatrollingPath->GetActorTransform(); // rot, loc, scale

	FVector ClosestWayp;
	float MinSqrDist = FLT_MAX;
	for (int32 i = 0; i < Wayps.Num(); ++i)
	{
		FVector WaypWorld = PathTransform.TransformPosition(Wayps[i]);
		float CurrentSqrDist = (OwnerLocation - WaypWorld).SizeSquared();
		if (CurrentSqrDist < MinSqrDist)
		{
			MinSqrDist = CurrentSqrDist;
			ClosestWayp = WaypWorld;
			CurrentWayPointIndex = i;
		}
	}

	return ClosestWayp;
}

FVector UAIPatrollingComponent::SelectNextWayPoint()
{
	const TArray<FVector> Wayps = PatrollingPath->GetWayPoints();
	FTransform PathTransform = PatrollingPath->GetActorTransform(); // rot, loc, scale

	if (bIsPingPongMode) // 0 - 1 - 2 - 1 - 0
	{
		if (!bIsGoingBack)
		{
			++CurrentWayPointIndex;
			if (CurrentWayPointIndex == Wayps.Num())
			{
				bIsGoingBack = true;
				CurrentWayPointIndex -= 2;
			}
		}
		else {
			--CurrentWayPointIndex;
			if (CurrentWayPointIndex < 0)
			{
				bIsGoingBack = false;
				CurrentWayPointIndex += 2;
			}
		}

	} else { // 0 - 1 - 2 - 0
		++CurrentWayPointIndex;
		if (CurrentWayPointIndex == Wayps.Num())
			CurrentWayPointIndex = 0;
	}

	return PathTransform.TransformPosition(Wayps[CurrentWayPointIndex]);
}
