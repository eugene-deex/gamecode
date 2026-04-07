#pragma once

#include "CoreMinimal.h"
#include "Actors/Interactive/Pickables/PickableItem.h"
#include "HomeWorkTypes.h"
#include "PickableAmmo.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class HOMEWORK1_API APickableAmmo : public APickableItem
{
	GENERATED_BODY()
public:
	APickableAmmo();

	virtual void Interact(ABaseCharacter* Char) override;
	virtual FName GetActionEventName() const override { return ActionInteract; };
	static bool CreateAndGiveAmmo(FName ID, ABaseCharacter* Char);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Scene;
};
