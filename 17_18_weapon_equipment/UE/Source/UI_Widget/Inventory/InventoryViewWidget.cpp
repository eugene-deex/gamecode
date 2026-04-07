#include "InventoryViewWidget.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "InventorySlotWidget.h"
#include "Components/GridPanel.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Utils/DeexUtils.h"

void UInventoryViewWidget::InitViewWidget(UCharacterEquipmentComponent* EquipComp, TArray<FInventorySlot>& InvSlots)
{
	LinkedEquipComp = EquipComp;

	for (FInventorySlot& InvSlot : InvSlots)
		AddItemSlotView(InvSlot);

	if (!EquipComp->UpdateInventoryEvent.IsBound())
		EquipComp->UpdateInventoryEvent.AddUFunction(this, FName("UpdateInventory"));
}

int32 UInventoryViewWidget::GetAmmo(EAmmunitionType AmmoType) const
{
	int32 AmmoTotal = 0;

	const int32 ChildrenCount = GridPanelItemSlots->GetChildrenCount();
	if (!ChildrenCount)
		return AmmoTotal;

	for (int32 i = ChildrenCount - 1; i >= 0; --i)
	{
		UWidget* Child = GridPanelItemSlots->GetChildAt(i);
		if (Child == nullptr || !IsValid(Child))
			continue;

		UInventorySlotWidget* EquipmentSlot = Cast<UInventorySlotWidget>(Child);
		if (!EquipmentSlot || !EquipmentSlot->IsValidLowLevelFast())
			continue;

		if (EquipmentSlot->GetAmmoType() == AmmoType)
			AmmoTotal += EquipmentSlot->GetCount();
	}

	return AmmoTotal;
}

void UInventoryViewWidget::UpdateInventory(FName WeaponID, int32 AmmoType, int32 OpCount, bool bIsAdding)
{
	int32 Count = OpCount;

	if (bIsAdding) // do nothing
		return;

	const int32 ChildrenCount = GridPanelItemSlots->GetChildrenCount();
	for (int32 i = ChildrenCount-1; i >=0; --i)
	{
		if (Count == 0)
			break;

		UWidget* Child = GridPanelItemSlots->GetChildAt(i);
		if (Child == nullptr || !IsValid(Child))
			continue;

		UInventorySlotWidget* EquipmentSlot = Cast<UInventorySlotWidget>(Child);
		if (!EquipmentSlot || !EquipmentSlot->IsValidLowLevelFast())
			continue;
		
		if(!EquipmentSlot->IsAmmo())
			continue;

		FName SlotItemID = EquipmentSlot->GetItemID();
		int32 NumberInSlot = EquipmentSlot->GetCount();

		if (SlotItemID != WeaponID)
			continue;

		if (Count > NumberInSlot) // clear the slot
		{
			Count -= NumberInSlot;
			if (Count < 0)
				Count = 0;
			EquipmentSlot->ClearSlot();
		
		}else{ // update the slot

			NumberInSlot -= Count;
			Count = 0;

			if(NumberInSlot == 0)
				EquipmentSlot->ClearSlot();
			else
				EquipmentSlot->SetCount(NumberInSlot);
		}

		EquipmentSlot->UpdateView();
	}
}

void UInventoryViewWidget::AddItemSlotView(FInventorySlot& SlotAdd)
{
	checkf(InvSlotWidgetClass.Get() != nullptr, TEXT("UInventoryViewWidget::AddItemSlotView widget slot class is not set"));

	UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, InvSlotWidgetClass);
	if (SlotWidget == nullptr)
		return;

	SlotWidget->InitItemSlot(SlotAdd);

	const int32 Total = GridPanelItemSlots->GetAllChildren().Num();
	const int32 Row = Total / Columns;
	const int32 Col = Total % Columns;
	GridPanelItemSlots->AddChildToGrid(SlotWidget, Row, Col);

	SlotWidget->UpdateView();
}
