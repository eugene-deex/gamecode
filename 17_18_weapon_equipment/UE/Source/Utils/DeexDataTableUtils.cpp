#include "DeexDataTableUtils.h"
#include "Engine/DataTable.h"
#include "HomeWorkTypes.h"
#include "Inventory/Items/InventoryItem.h"

FWeaponTableRow* DeexDataTableUtils::FindWeaponData(const FName ID)
{
	static const FString ctxString(TEXT("Find Weapon Data"));
	// TODO: hardcoded path to the data table
	UDataTable* table = LoadObject<UDataTable>(nullptr, TEXT("/Game/HomeWork1/Core/Data/DataTables/DT_WeaponList.DT_WeaponList"));
	return table != nullptr ? table->FindRow<FWeaponTableRow>(ID, ctxString) : nullptr;
}

FItemTableRow* DeexDataTableUtils::FindInvItemData(const FName ID)
{
	static const FString ctxString(TEXT("Find Item Data"));

	UDataTable* table = LoadObject<UDataTable>(nullptr, TEXT("/Game/HomeWork1/Core/Data/DataTables/DT_InventoryItemList.DT_InventoryItemList"));
	return table != nullptr ? table->FindRow<FItemTableRow>(ID, ctxString) : nullptr;
}

FAmmoTableRow* DeexDataTableUtils::FindAmmoData(const FName ID)
{
	static const FString ctxString(TEXT("Find Ammo Data"));

	UDataTable* table = LoadObject<UDataTable>(nullptr, TEXT("/Game/HomeWork1/Core/Data/DataTables/DT_AmmoList.DT_AmmoList"));
	if (!table)
		return nullptr;

	return table->FindRow<FAmmoTableRow>(ID, ctxString);
}
