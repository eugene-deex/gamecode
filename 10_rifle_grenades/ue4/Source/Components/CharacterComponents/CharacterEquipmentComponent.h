#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "Actors/Equipment/Throwables/ThrowableItem.h"
#include "HomeWorkTypes.h"
#include "CharacterEquipmentComponent.generated.h"

typedef TArray<int32, TInlineAllocator<(uint32)EAmunitionType::MAX>> TAmmunitionArray; // keeep current ammos
typedef TArray<class AEquipableItem*, TInlineAllocator<(uint32)EEquipmentSlots::MAX>> TItemsArray; // keep current weapon

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCurrentWeaponAmmoChangedEvent, int32, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGrenadesAmmoChangedEvent, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChangedEvent, const AEquipableItem*);

class ARangeWeaponItem;
class AThrowableItem;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORK1_API UCharacterEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ARangeWeaponItem* GetCurrentRangeWeapon() const;
	EEquipableItemType GetCurrentEquippedItemType() const;
	
	FOnCurrentWeaponAmmoChangedEvent OnCurrentWeaponAmmoChangedEvent;
	FOnGrenadesAmmoChangedEvent OnGrenadesAmmoChangedEvent;
	FOnEquippedItemChangedEvent OnEquippedItemChangedEvent;

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
	void ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo = 0, bool bCheckIsFull = false);
	void SwitchWeaponMode();
	int32 GetAvailableAmmoForCurWeapon() const;

protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EAmunitionType, int32> MaxAmunitionAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EEquipmentSlots, TSubclassOf<class AEquipableItem>> ItemsLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TSet<EEquipmentSlots> IgnoreSlotsForSwitch; // do not change back to these slots after throwable has been thrown

private:
	TAmmunitionArray AmmunitionArray;
	TItemsArray ItemsArray;

	void CreateLoadout(); //create current weapon, etc; Loadout - equipamiento, ekipirovka

	void EquipAnimFinished();

	uint32 NextItemsArraySlotIndex(uint32 CurrentSlotIndex);
	uint32 PrevItemsArraySlotIndex(uint32 CurrentSlotIndex);

	UFUNCTION()
	void OnWeaponReloadComplete();

	UFUNCTION()
	void OnCurrentWeaponAmmoChanged(int32 NewAmmo);

	UFUNCTION()
	void OnGrenadesAmmoChanged(int32 NewGrenades);

	AEquipableItem* CurrentEquippedItem;
	
	EEquipmentSlots PrevEquippedSlot;
	EEquipmentSlots CurrentEquippedSlot;

	ARangeWeaponItem* CurrentEquippedWeapon;
	AThrowableItem* CurrentThrowable;

	TWeakObjectPtr<class ABaseCharacter> CachedChar;

	FDelegateHandle OnAmmoChangeHnd;
	FDelegateHandle OnGrenadesChangeHnd;
	FDelegateHandle OnReloadHnd;

	bool bIsEquipping = false;
	FTimerHandle EquipTimer;
};
