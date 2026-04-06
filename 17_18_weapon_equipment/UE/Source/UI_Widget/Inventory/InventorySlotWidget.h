#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Items/InventoryItem.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "Inventory/Items/Equipables/AmmoItem.h"
#include "Utils/DeexUtils.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class HOMEWORK1_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	FInventorySlot* GetLinkedSlot() const {
		return LinkedSlot;
	}

	void InitItemSlot(FInventorySlot& Slot);
	void UpdateView(); // used in FInventorySlot callback
	void SetItemIcon(UTexture2D* Icon);
	bool IsAmmo() const {
		UAmmoItem* Ptr = Cast<UAmmoItem>(LinkedSlot->Item);
		return IsValid(Ptr);
	}

	void SetCountText(int32 Count);
	void SetCount(int32 Count) {
		SetCountText(Count);
		LinkedSlot->Count = Count;
	};
	void ClearSlot() { 
		LinkedSlot->ClearSlot(); 
	}
	FName GetItemID() const {

		if(!LinkedSlot || !LinkedSlot->Item || LinkedSlot->Item->IsPendingKill())
			return NAME_None;

		return LinkedSlot->Item->GetID();
	}

	int32 GetCount() const {
		if (!LinkedSlot || !LinkedSlot->Item || LinkedSlot->Item->IsPendingKill())
			return 0;

		return LinkedSlot->Count;
	}

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* ImageItemIcon;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CountText;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	FInventorySlot* LinkedSlot;
};
