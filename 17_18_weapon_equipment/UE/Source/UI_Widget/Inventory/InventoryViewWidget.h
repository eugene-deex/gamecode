#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HomeWorkTypes.h"
#include "InventoryViewWidget.generated.h"

struct FInventorySlot;
class UInventorySlotWidget;
class UGridPanel;
class UCharacterEquipmentComponent;
class AEquipableItem;

UCLASS()
class HOMEWORK1_API UInventoryViewWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitViewWidget(UCharacterEquipmentComponent* EquipComp, TArray<FInventorySlot>& InvSlots);
	int32 GetAmmo(EAmmunitionType AmmoType) const;

protected:
	UPROPERTY(meta = (BindWidget))
	UGridPanel* GridPanelItemSlots;

	UPROPERTY(EditDefaultsOnly, Category = "ItemContainer View Settings")
	TSubclassOf<UInventorySlotWidget> InvSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "ItemContainer View Settings")
	int32 Columns = 4;

	void AddItemSlotView(FInventorySlot& SlotAdd);

private:
	UCharacterEquipmentComponent* LinkedEquipComp;

	UFUNCTION()
	void UpdateInventory(FName WeaponID, int32 AmmoType, int32 Count, bool bIsAdding);
};
