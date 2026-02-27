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

class ABaseCharacter;
class ARangeWeaponItem;
class AThrowableItem;
class AMeleeWeaponItem;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOMEWORK1_API UCharacterEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCharacterEquipmentComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	AMeleeWeaponItem* GetCurrentMeleeWeapon() const;
	ABaseCharacter* GetCharacter() const { return CachedChar.Get(); }

protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EAmunitionType, int32> MaxAmunitionAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EEquipmentSlots, TSubclassOf<class AEquipableItem>> ItemsLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TSet<EEquipmentSlots> IgnoreSlotsForSwitch; // do not change back to these slots after throwable has been thrown

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	EEquipmentSlots AutoEquipItemInSlot = EEquipmentSlots::None;
	
private:
	TWeakObjectPtr<class ABaseCharacter> CachedChar;

	// rep funcs
	UFUNCTION(Server, Reliable)
	void Srv_EquipItemInSlot(EEquipmentSlots Slot);
	
	UPROPERTY(Replicated)
	TArray<int32> AmmunitionArray; // TAmmunitionArray cannot be replicated
	
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
	void OnCurrentWeaponAmmoChanged(int32 NewAmmo);

	UFUNCTION()
	void OnGrenadesAmmoChanged(int32 NewGrenades);

	AEquipableItem* CurrentEquippedItem;
	
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
};
