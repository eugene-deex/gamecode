#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "AmmoItem.generated.h"

UCLASS()
class HOMEWORK1_API UAmmoItem : public UInventoryItem
{
	GENERATED_BODY()
public:
	UAmmoItem();

	void SetAmmoType(EAmmunitionType Type_) { Type = Type_; };
	EAmmunitionType GetAmmoType() const { return Type; };

	void SetCount(int32 Count_) { Count = Count_; };
	int32 GetCount() const { return Count; };

protected:
	EAmmunitionType Type;
	int32 Count;
};
