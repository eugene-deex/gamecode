#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"

namespace DeexDataTableUtils
{
	FWeaponTableRow* FindWeaponData(const FName ID);
	FItemTableRow* FindInvItemData(const FName ID);
	FAmmoTableRow* FindAmmoData(const FName ID);
};
