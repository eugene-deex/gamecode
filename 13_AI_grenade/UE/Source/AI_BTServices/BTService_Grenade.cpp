#include "BTService_Grenade.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/BaseCharacter.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"

UBTService_Grenade::UBTService_Grenade()
{
	NodeName = "Grenade";
}

void UBTService_Grenade::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	CachedOwner = &OwnerComp;

	AAIController* AICtrl = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();

	if (!IsValid(AICtrl) || !IsValid(BBComp))
		return;

	if (BBComp->GetValueAsBool(IsThrowing.SelectedKeyName))
	{
		UE_LOG(LogDamage, Warning, TEXT("GRENADE IS COOLING DOWN"));
		return;
	}
	
	APawn* Pawn = AICtrl->GetPawn();
	checkf(Pawn->IsA<ABaseCharacter>(), TEXT("UBTService_Grenade::TickNode accepts only ABaseCharacter as an owner"));

	ABaseCharacter* Char = StaticCast<ABaseCharacter*>(Pawn);
	if (!IsValid(Char))
		return;

	UCharacterEquipmentComponent* EquipComp = Char->GetEquipmentComp_Mutable();

	AActor* CurrentTarget = Cast<AActor>(BBComp->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(CurrentTarget))
		return;

	float DistSq = FVector::DistSquared(CurrentTarget->GetActorLocation(), Char->GetActorLocation());
	
	if (DistSq < FMath::Square(MinFireDistance) || DistSq > FMath::Square(MaxFireDistance))
		return;

	AICtrl->StopMovement();
	Char->EquipPrimaryItem();
	EquipComp->LaunchCurrentThrowable();
	
	BBComp->SetValueAsBool(IsThrowing.SelectedKeyName, true);
	
	GetWorld()->GetTimerManager().ClearTimer(CooldownTimer);
	GetWorld()->GetTimerManager().SetTimer(CooldownTimer, this, &UBTService_Grenade::ResetCooldown, CooldownTime, false);
	UE_LOG(LogDamage, Warning, TEXT("GRENADE THROWN"));
}

void UBTService_Grenade::ResetCooldown()
{
	if (!CachedOwner.IsValid())
		return;

	UBlackboardComponent* BBComp = CachedOwner->GetBlackboardComponent();

	if (!IsValid(BBComp))
		return;

	BBComp->SetValueAsBool(IsThrowing.SelectedKeyName, false);
}