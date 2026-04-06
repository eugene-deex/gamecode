#include "EquipmentViewWidget.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Components/HorizontalBox.h"
#include "EquipmentSlotWidget.h"
#include "Actors/Interactive/Pickables/PickableItem.h"

void UEquipmentViewWidget::InitEquipmentWidget(UCharacterEquipmentComponent* EquipComp)
{
	LinkedEquipComp = EquipComp;
	const TArray<AEquipableItem*>& Items = EquipComp->GetItems();

	// skip index 0 because first EEquipmentSlots enum is None
	for (int32 Index = 1; Index < Items.Num(); ++Index)
		AddEquipmentSlotView(Items[Index], Index);

	if (!EquipComp->OnAmmoAddedEvent.IsBound())
		EquipComp->OnAmmoAddedEvent.AddUFunction(this, FName("AmmoAddedHandler"));
}

void UEquipmentViewWidget::AmmoAddedHandler(TSubclassOf<AEquipableItem> WeaponType, int32 Total, int32 InWeaponCount)
{
	if (!WeaponSlots) return;

	const int32 ChildrenCount = WeaponSlots->GetChildrenCount();
	for (int32 i = 0; i < ChildrenCount; ++i)
	{
		UWidget* Child = WeaponSlots->GetChildAt(i);
		if (Child == nullptr || !IsValid(Child))
			continue;

		UEquipmentSlotWidget* EquipmentSlot = Cast<UEquipmentSlotWidget>(Child);
		if (EquipmentSlot && EquipmentSlot->IsValidLowLevelFast())
		{
			TSubclassOf<AEquipableItem> SlotWeaponClass = EquipmentSlot->GetWeaponType();
			if(SlotWeaponClass == WeaponType)
			{
				EquipmentSlot->SetCurrentAmmoCount(InWeaponCount);
				EquipmentSlot->SetAmmoCount(Total);
				return;
			}
		}
	}
}

void UEquipmentViewWidget::UpdateCounters()
{
	if (!WeaponSlots) return;

	const int32 ChildrenCount = WeaponSlots->GetChildrenCount();
	for (int32 i = 0; i < ChildrenCount; ++i)
	{
		UWidget* Child = WeaponSlots->GetChildAt(i);
		if(Child == nullptr || !IsValid(Child))
			continue;

		UEquipmentSlotWidget* EquipmentSlot = Cast<UEquipmentSlotWidget>(Child);
		if (EquipmentSlot && EquipmentSlot->IsValidLowLevelFast())
			EquipmentSlot->UpdateCounter();
	}
}

void UEquipmentViewWidget::AddEquipmentSlotView(AEquipableItem* LinkToWeapon, int32 SlotIndex)
{
	checkf(DefaultSlotViewClass.Get() != nullptr, TEXT("UEquipmentViewWidget::AddEquipmentSlotView DefaultSlotViewClass is not set"));

	UEquipmentSlotWidget* SlotWidget = CreateWidget<UEquipmentSlotWidget>(this, DefaultSlotViewClass);
	if (!IsValid(SlotWidget))
		return;

	SlotWidget->InitEquipmentSlot(LinkToWeapon, SlotIndex, LinkedEquipComp);
	WeaponSlots->AddChildToHorizontalBox(SlotWidget);
	SlotWidget->UpdateView();
	SlotWidget->OnEquipmentDropInSlot.BindUObject(this, &UEquipmentViewWidget::EquipEquipmentToSlot);
	SlotWidget->OnEquipmentRemovedFromSlot.BindUObject(this, &UEquipmentViewWidget::RemoveEquipmentToSlot);
}

void UEquipmentViewWidget::UpdateSlot(int32 SlotIndex)
{
	UEquipmentSlotWidget* WidgetToUpdate = Cast<UEquipmentSlotWidget>(WeaponSlots->GetChildAt(SlotIndex - 1));
	if (!IsValid(WidgetToUpdate))
		return;

	WidgetToUpdate->InitEquipmentSlot(LinkedEquipComp->GetItems()[SlotIndex], SlotIndex, LinkedEquipComp);
	WidgetToUpdate->UpdateView();
}

bool UEquipmentViewWidget::EquipEquipmentToSlot(const TSubclassOf<AEquipableItem>& WeaponClass, int32 SenderIndex)
{
	const bool Result = LinkedEquipComp->AddEquipmentItemToSlot(WeaponClass, SenderIndex);
	if (Result)
		UpdateSlot(SenderIndex);

	return Result;
}

void UEquipmentViewWidget::RemoveEquipmentToSlot(int32 SlotIndex)
{
	LinkedEquipComp->RemoveItemFromSlot(SlotIndex);
}
