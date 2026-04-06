#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "Medkit.generated.h"

UCLASS()
class HOMEWORK1_API UMedkit : public UInventoryItem
{
	GENERATED_BODY()
	
public:
	virtual bool Consume(ABaseCharacter* Consumer) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Medkit")
	float Health = 25.f;

};
