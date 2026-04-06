#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "Adrenaline.generated.h"

UCLASS()
class HOMEWORK1_API UAdrenaline : public UInventoryItem
{
	GENERATED_BODY()
	
public:
	virtual bool Consume(ABaseCharacter* Consumer) override;
};
