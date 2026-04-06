#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Actors/Interactive/Pickables/PickableItem.h"
#include "Actors/Equipment/EquipableItem.h"
#include "InventoryItem.generated.h"

class UInventoryItem;

USTRUCT(BlueprintType)
struct FInventoryItemDesc : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item View")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item View")
	UTexture2D* Icon;
};

USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon View")
	TSubclassOf<APickableItem> PickableActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon View")
	TSubclassOf<AEquipableItem> EquipableActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon View")
	FInventoryItemDesc WeaponItemDesc;
};

USTRUCT(BlueprintType)
struct FAmmoTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo View")
	TSubclassOf<APickableItem> PickableActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo View")
	EAmmunitionType AmmunitionType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo View")
	FInventoryItemDesc ItemDesc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo View")
	int32 Count;
};

USTRUCT(BlueprintType)
struct FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item View")
	TSubclassOf<APickableItem> PickableActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item View")
	TSubclassOf<UInventoryItem> InvItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item View")
	FInventoryItemDesc Desc;
};

UCLASS(Blueprintable)
class HOMEWORK1_API UInventoryItem : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(FName ID_In, const FInventoryItemDesc& Desc_In) {
		ID = ID_In;
		Desc.Icon = Desc_In.Icon;
		Desc.Name = Desc_In.Name;
		bIsInitialized = true;
	}

	FName GetID() const { return ID; };
	const FInventoryItemDesc& GetDescription() const { return Desc; };

	virtual bool IsEquipable() const { return bIsEquipable; };
	virtual bool IsConsumable() const { return bIsConsumable; };

	virtual bool Consume(ABaseCharacter* Consumer) PURE_VIRTUAL(UInventoryItem::Consume, return false;);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Item")
	FName ID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Item")
	FInventoryItemDesc Desc;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Item")
	bool bIsEquipable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Item")
	bool bIsConsumable = false;

private:
	bool bIsInitialized = false;

};
