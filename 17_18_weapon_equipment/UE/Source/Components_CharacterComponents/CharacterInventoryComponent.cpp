#include "CharacterInventoryComponent.h"
#include "UI/Widget/Inventory/InventoryViewWidget.h"
#include "Inventory/Items/InventoryItem.h"
#include "Characters/BaseCharacter.h"
#include "Characters/Controllers/BasePlayerController.h"

UCharacterInventoryComponent::UCharacterInventoryComponent()
{
	InvSlots.AddDefaulted(Capacity);
}

void UCharacterInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedChar = StaticCast<ABaseCharacter*>(GetOwner());

	// create inventory widget
	if (!InvWidget)
	{
		ABasePlayerController* Ctrl = CachedChar->GetController<ABasePlayerController>();
		if (Ctrl && Ctrl->IsPlayerController())
			CreateViewWidget(Ctrl);
	}
}

void UCharacterInventoryComponent::CreateViewWidget(APlayerController* Ctrl)
{
	if (IsValid(InvWidget) || !IsValid(Ctrl) || !IsValid(InvWidgetClass))
		return;

	InvWidget = CreateWidget<UInventoryViewWidget>(Ctrl, InvWidgetClass);
	ABaseCharacter* Char = Cast<ABaseCharacter>(GetOwner());
	InvWidget->InitViewWidget(Char->GetEquipmentComp_Mutable(), InvSlots);
}

FInventorySlot* UCharacterInventoryComponent::FindSlotByID(FName ID)
{
	return InvSlots.FindByPredicate([=](const FInventorySlot Slot) {
		return Slot.Item && Slot.Item->GetID() == ID;
	});
}

FInventorySlot* UCharacterInventoryComponent::FindFreeSlot()
{
	return InvSlots.FindByPredicate([=](const FInventorySlot Slot) {
		return !Slot.Item;
	});
}

void UCharacterInventoryComponent::OpenInventory(APlayerController* Ctrl)
{
	if (!IsValid(InvWidget))
		CreateViewWidget(Ctrl);

	if (IsValid(InvWidget) && !InvWidget->IsVisible())
	{
		InvWidget->AddToViewport();
	}
}

void UCharacterInventoryComponent::CloseInventory()
{
	if (InvWidget->IsVisible())
		InvWidget->RemoveFromParent();
}

TArray<FName> UCharacterInventoryComponent::GetAllNames() const
{
	TArray<FName> Names;
	for (const FInventorySlot& Slot : InvSlots)
		if(Slot.Item)
			Names.Add(Slot.Item->GetDescription().Name);

	return Names;
}

bool UCharacterInventoryComponent::AddItem(UInventoryItem* Item, int32 Count)
{
	if (!Item || Count < 0)
		return false;

	FInventorySlot* Slot = FindFreeSlot();
	if (!HasFreeSlot() || Slot == nullptr) // TODO: find a slot with the same item and add Count to it
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("No free space in the inventory"));
		return false;
	}

	Slot->Item = Item;
	Slot->Count = Count;
	Slot->UpdateSlotState(); // send delegate to callback
	InvItemsCount++;

	return true;
}

bool UCharacterInventoryComponent::Remove(FName ID)
{
	FInventorySlot* Slot = FindSlotByID(ID);
	if (Slot == nullptr)
		return false;

	InvSlots.RemoveAll([=](const FInventorySlot& Slot) { 
		if (Slot.Item && Slot.Item->GetID() == ID)
		{
			InvItemsCount -= Slot.Count;
			return true;
		}
		return false;
	});

	return true;
}

void FInventorySlot::ClearSlot()
{
	Item = nullptr;
	Count = 0;
	UpdateSlotState();
}
