#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "UI/Widget/ItemDragOp.h"
#include "Actors/Equipment/EquipableItem.h"
#include "EquipmentSlotWidget.generated.h"

class UImage;
class UInventorySlotWidget;
class UWeaponItem;
class UCharacterEquipmentComponent;
class APickableItem;

UCLASS()
class HOMEWORK1_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnEquipmentDropInSlot, const TSubclassOf<AEquipableItem>&, int32);
	DECLARE_DELEGATE_OneParam(FOnEquipmentRemovedFromSlot, int32);

	FOnEquipmentDropInSlot OnEquipmentDropInSlot;
	FOnEquipmentRemovedFromSlot OnEquipmentRemovedFromSlot;

	void InitEquipmentSlot(TWeakObjectPtr<AEquipableItem> Equipment, int32 Index, TWeakObjectPtr<UCharacterEquipmentComponent> EquipComp);
	void SetAmmoCount(int32 Count) { TextAmmoCount->SetText(FText::AsNumber(Count)); }
	void SetCurrentAmmoCount(int32 Count) { TextCurrentAmmoCount->SetText(FText::AsNumber(Count)); }
	void UpdateView();
	void UpdateCounter();
	TSubclassOf<AEquipableItem> GetWeaponType() const { return LinkedEquipableItem->GetClass(); };

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* ImageWeaponIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TBWeaponName;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextAmmoCount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextCurrentAmmoCount;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UInventorySlotWidget> DragAndDropWidgetClass;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	AEquipableItem* LinkedEquipableItem;

	UPROPERTY()
	UWeaponItem* AdapterLinkedWeaponItem;

	int32 SlotIndexInComp = 0;
	TWeakObjectPtr<UCharacterEquipmentComponent> EquipComp;
};

