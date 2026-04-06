#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "WeaponItem.generated.h"

class AEquipableItem;

UCLASS()
class HOMEWORK1_API UWeaponItem : public UInventoryItem
{
	GENERATED_BODY()
	
public:
	UWeaponItem();

	void SetEquipWeaponClass(TSubclassOf<AEquipableItem> Class) { EquipWeaponClass = Class; };
	TSubclassOf<AEquipableItem> GetEquipWeaponClass() const { return EquipWeaponClass; };

protected:
	TSubclassOf<AEquipableItem> EquipWeaponClass;
};
