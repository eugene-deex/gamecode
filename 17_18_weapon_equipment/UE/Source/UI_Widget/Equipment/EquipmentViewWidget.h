#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EquipmentViewWidget.generated.h"

class UCharacterEquipmentComponent;
class AEquipableItem;
class UHorizontalBox;
class UEquipmentSlotWidget;

UCLASS()
class HOMEWORK1_API UEquipmentViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitEquipmentWidget(UCharacterEquipmentComponent* EquipComp);
	void UpdateCounters();

protected:
	void AddEquipmentSlotView(AEquipableItem* LinkToWeapon, int32 SlotIndex);
	void UpdateSlot(int32 SlotIndex);
	bool EquipEquipmentToSlot(const TSubclassOf<AEquipableItem>& WeaponClass, int32 SenderIndex);
	void RemoveEquipmentToSlot(int32 SlotIndex);

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* WeaponSlots;

	UPROPERTY(EditDefaultsOnly, Category = "ItemContainer View Settings")
	TSubclassOf<UEquipmentSlotWidget> DefaultSlotViewClass;

	TWeakObjectPtr<UCharacterEquipmentComponent> LinkedEquipComp;

private:

	UFUNCTION()
	void AmmoAddedHandler(TSubclassOf<AEquipableItem> WeaponType, int32 Total, int32 InWeaponCount);
};
