#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "Actors/Equipment/Throwables/ThrowableItem.h"
#include "HomeWorkTypes.h"
#include "UI/Widget/Equipment/EquipmentViewWidget.h"
#include "CharacterEquipmentComponent.generated.h"

class ABaseCharacter;
class ARangeWeaponItem;
class AThrowableItem;
class AMeleeWeaponItem;
class AEquipableItem;
class APickableItem;

typedef TArray<class AEquipableItem*, TInlineAllocator<(uint32)EEquipmentSlots::MAX>> TItemsArray; // keep current weapon

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCurrentWeaponAmmoChangedEvent, int32, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGrenadesAmmoChangedEvent, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChangedEvent, const AEquipableItem*);

// to update equipment slots counters on each shot; 1 - EAmmunitionType, 2 - number of bullets
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAmmoAddedEvent, TSubclassOf<AEquipableItem>, int32, int32);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAmmoRemovedEvent, TSubclassOf<AEquipableItem>, int32, int32);
DECLARE_MULTICAST_DELEGATE_FourParams(FUpdateInventoryEvent, FName, int32, int32, bool);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORK1_API UCharacterEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	TMap<TSubclassOf<AEquipableItem>, EAmmunitionType> WeaponAmmoTypes;

	UCharacterEquipmentComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ARangeWeaponItem* GetCurrentRangeWeapon() const;
	EEquipableItemType GetCurrentEquippedItemType() const;
	
	FOnCurrentWeaponAmmoChangedEvent OnCurrentWeaponAmmoChangedEvent;
	FOnGrenadesAmmoChangedEvent OnGrenadesAmmoChangedEvent;
	FOnEquippedItemChangedEvent OnEquippedItemChangedEvent;
	
	FOnAmmoAddedEvent OnAmmoAddedEvent;
	FOnAmmoRemovedEvent OnAmmoRemovedEvent;
	
	FUpdateInventoryEvent UpdateInventoryEvent;

	void ReloadCurrentWeapon();
	void EquipItemSlot(EEquipmentSlots Slot);
	void AttachCurrentItem();
	void UnEquipCurrentItem();
	
	void EquipNextItem();
	void EquipPreviousItem();
	//void EquipSideArm();
	//void EquipPrimaryWeapon();
	//void EquipSecondaryWeapon();
	void LaunchCurrentThrowable();

	bool IsEquipping() const { return bIsEquipping; };
	void ReloadAmmoInCurrentWeapon(uint32 NumberOfAmmo = 0, bool bCheckIsFull = false);
	void SwitchWeaponMode();
	uint32 GetAvailableAmmoForCurWeapon() const;

	uint32 GetCurrentAmmoByWeaponType(TSubclassOf<AEquipableItem> WeaponClass) const;

	AMeleeWeaponItem* GetCurrentMeleeWeapon() const;
	ABaseCharacter* GetCharacter() const { return CachedChar.Get(); }

	bool AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem> EquipableItemClass, int32 SlotIndex);
	bool AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem> EquipableItemClass, int32 SlotIndex, AEquipableItem*& SpawnedItem);
	void RemoveItemFromSlot(int32 SlotIndex);

	void OpenViewEquipment(APlayerController* Ctrl);
	void CloseViewEquipment() { if(IsViewVisible()) ViewWidget->RemoveFromParent(); };
	bool IsViewVisible() const;
	void SyncEquipmentWidget() {
		if (ViewWidget && IsViewVisible())
		{
			ViewWidget->UpdateCounters();
		}
	}
	const TArray<AEquipableItem*> GetItems() const { return ItemsArray; };
	
	int32 GetAmmo(EAmmunitionType AmmoType) const;

	EAmmunitionType GetAmmoTypeByWeaponType(TSubclassOf<AEquipableItem> WeaponClass) const;
	void AddAmmo(EAmmunitionType AmmoType, uint32 Count, FName WeaponID);
	//void RemoveAmmo(EAmmunitionType AmmoType, uint32 Count, TSubclassOf<APickableItem> PickableActor);

protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EAmmunitionType, int32> MaxAmunitionAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EEquipmentSlots, TSubclassOf<class AEquipableItem>> ItemsLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TSet<EEquipmentSlots> IgnoreSlotsForSwitch; // do not change back to these slots after throwable has been thrown

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	EEquipmentSlots AutoEquipItemInSlot = EEquipmentSlots::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View")
	TSubclassOf<UEquipmentViewWidget> ViewWidgetClass;

	void CreateViewWidget(APlayerController* Ctrl);

private:
	TWeakObjectPtr<class ABaseCharacter> CachedChar;

	// rep funcs
	UFUNCTION(Server, Reliable)
	void Srv_EquipItemInSlot(EEquipmentSlots Slot);
		
	UPROPERTY(ReplicatedUsing=OnRep_ItemsArray)
	TArray<AEquipableItem*> ItemsArray;

	UFUNCTION()
	void OnRep_ItemsArray();

	void CreateLoadout(); //create current weapon, etc; Loadout - equipamiento, ekipirovka

	void AutoEquip();
	void EquipAnimFinished();

	uint32 NextItemsArraySlotIndex(uint32 CurrentSlotIndex);
	uint32 PrevItemsArraySlotIndex(uint32 CurrentSlotIndex);

	UFUNCTION()
	void OnWeaponReloadComplete();

	UFUNCTION()
	void OnCurrentWeaponAmmoChanged(uint32 NewAmmo);

	UFUNCTION()
	void OnGrenadesAmmoChanged(uint32 NewGrenades);

	TWeakObjectPtr<AEquipableItem> CurrentEquippedItem;
	
	EEquipmentSlots PrevEquippedSlot;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentEquippedSlot)
	EEquipmentSlots CurrentEquippedSlot;

	UFUNCTION()
	void OnRep_CurrentEquippedSlot(EEquipmentSlots CurEquipSlotOld);

	ARangeWeaponItem* CurrentEquippedWeapon;
	AThrowableItem* CurrentThrowable;
	AMeleeWeaponItem* CurrentMeleeWeapon;

	FDelegateHandle OnAmmoChangeHnd;
	FDelegateHandle OnGrenadesChangeHnd;
	FDelegateHandle OnReloadHnd;

	bool bIsEquipping = false;
	FTimerHandle EquipTimer;

	UPROPERTY()
	UEquipmentViewWidget* ViewWidget;

	TSubclassOf<AEquipableItem> GetWeaponClassByAmmoType(EAmmunitionType AmmoType);
};
