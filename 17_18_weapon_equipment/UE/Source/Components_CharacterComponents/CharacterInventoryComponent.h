#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/Widget/Inventory/InventoryViewWidget.h"
#include "Components/ActorComponent.h"
#include "CharacterInventoryComponent.generated.h"

class UInventoryItem;
//class UInventoryViewWidget;

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FInventorySlotUpdate); // fires on slot update
	void BindOnSlotUpdate(const FInventorySlotUpdate& Callback) const { OnInventorySlotUpdate = Callback; };
	void UnBindOnSlotUpdate() const { OnInventorySlotUpdate.Unbind(); };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UInventoryItem* Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0; // slot can contain multiple items of the same type

	void UpdateSlotState() { OnInventorySlotUpdate.ExecuteIfBound(); };
	void ClearSlot();

private:
	mutable FInventorySlotUpdate OnInventorySlotUpdate;
};

UCLASS()
class HOMEWORK1_API UCharacterInventoryComponent : public UActorComponent 
{
	GENERATED_BODY()
	
public:
	UCharacterInventoryComponent();
	void OpenInventory(APlayerController* Ctrl);
	void CloseInventory();

	bool IsViewVisible() const { return IsValid(InvWidget) && InvWidget->IsVisible(); };
	int32 GetCapacity() const { return Capacity; };
	bool HasFreeSlot() const { return InvItemsCount < Capacity; };
	TArray<FInventorySlot> GetItemsCopy() const { return InvSlots; };

	TArray<FName> GetAllNames() const;

	bool AddItem(UInventoryItem* Item, int32 Count);
	bool Remove(FName ID);

	int32 GetAmmo(EAmmunitionType AmmoType) const;

protected:
	UPROPERTY(EditAnywhere, Category = "Items")
	TArray<FInventorySlot> InvSlots;

	UPROPERTY(EditAnywhere, Category = "View Settings")
	TSubclassOf<UInventoryViewWidget> InvWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Settings", meta = (ClampMin = 1, UIMin = 1))
	int32 Capacity = 16;

	virtual void BeginPlay() override;
	void CreateViewWidget(APlayerController* Ctrl);

	FInventorySlot* FindSlotByID(FName ID);
	FInventorySlot* FindFreeSlot();

private:
	UPROPERTY()
	UInventoryViewWidget* InvWidget;

	TWeakObjectPtr<class ABaseCharacter> CachedChar;
	int32 InvItemsCount;
};
