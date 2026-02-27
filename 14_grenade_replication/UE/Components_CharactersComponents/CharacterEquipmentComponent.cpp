#include "CharacterEquipmentComponent.h"
#include "HomeWorkTypes.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Characters/BaseCharacter.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Actors/Equipment/Throwables/ThrowableItem.h"
#include "Actors/Equipment/Weapons/MeleeWeaponItem.h"
#include "Net/UnrealNetwork.h"
#include "HomeWork1.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "HomeworkTypes.h"
#include "Utils/DeexUtils.h"

UCharacterEquipmentComponent::UCharacterEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCharacterEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCharacterEquipmentComponent, CurrentEquippedSlot);
	DOREPLIFETIME(UCharacterEquipmentComponent, AmmunitionArray);
	DOREPLIFETIME(UCharacterEquipmentComponent, ItemsArray);
}

// return: pointer or null
ARangeWeaponItem* UCharacterEquipmentComponent::GetCurrentRangeWeapon() const
{
	return CurrentEquippedWeapon;
}

EEquipableItemType UCharacterEquipmentComponent::GetCurrentEquippedItemType() const
{
	return IsValid(CurrentEquippedItem) ? CurrentEquippedItem->GetItemType() : EEquipableItemType::None;
}

void UCharacterEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("UCharacterAttributesComponent::BeginPlay can be used only with ABaseCharacter"))
	CachedChar = StaticCast<ABaseCharacter*>(GetOwner());

	CreateLoadout();
	AutoEquip();
}

void UCharacterEquipmentComponent::OnRep_ItemsArray()
{
	for(AEquipableItem* Item : ItemsArray)
	{
		if (IsValid(Item))
		{
			Item->UnEquip();
		}
	}
}

void UCharacterEquipmentComponent::CreateLoadout()
{
	if (CachedChar->HasAuthority() || CachedChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ItemsArray.AddZeroed((uint32)EEquipmentSlots::MAX);
		for (const TPair<EEquipmentSlots, TSubclassOf<AEquipableItem>>& ItemPair : ItemsLoadout)
		{
			if (!IsValid(ItemPair.Value))
			{
				continue;
			}

			AEquipableItem* Item = GetWorld()->SpawnActor<AEquipableItem>(ItemPair.Value);
			// gives crash if called not on the server
			Item->AttachToComponent(CachedChar->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Item->GetUnEquippedSocketName());
			Item->SetOwner(CachedChar.Get());

			Item->UnEquip();
			ItemsArray[(uint32)ItemPair.Key] = Item;

			// set grenade number to HUD
			AThrowableItem* Throwable = Cast<AThrowableItem>(Item);
			if (IsValid(Throwable) && OnGrenadesAmmoChangedEvent.IsBound())
			{
				OnGrenadesAmmoChangedEvent.Broadcast(Throwable->GetAmmo(), Throwable->GetMaxAmmo());
			}
		}

		AmmunitionArray.AddZeroed((uint32)EAmunitionType::MAX);
		for (const TPair<EAmunitionType, int32>& AmmoPair : MaxAmunitionAmount)
		{
			AmmunitionArray[(uint32)AmmoPair.Key] = FMath::Max(AmmoPair.Value, 0);
		}

		EquipNextItem();
	}

}

void UCharacterEquipmentComponent::AutoEquip()
{
	if (AutoEquipItemInSlot != EEquipmentSlots::None)
	{
		EquipItemSlot(AutoEquipItemInSlot);
	}
}

void UCharacterEquipmentComponent::EquipAnimFinished()
{
	bIsEquipping = false;
	AttachCurrentItem();
}

uint32 UCharacterEquipmentComponent::NextItemsArraySlotIndex(uint32 CurrentSlotIndex)
{
	return (CurrentSlotIndex == ItemsArray.Num() - 1) ? 0 : CurrentSlotIndex + 1;
}

uint32 UCharacterEquipmentComponent::PrevItemsArraySlotIndex(uint32 CurrentSlotIndex)
{
	return (CurrentSlotIndex == 0) ? ItemsArray.Num() - 1 : CurrentSlotIndex - 1;
}

void UCharacterEquipmentComponent::ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo /*= 0*/, bool bCheckIsFull /*= false*/)
{
	int32 AvailAmmo = GetAvailableAmmoForCurWeapon(); // how much do we have
	if (AvailAmmo <= 0)
	{
		return;
	}

	int32 CurrentAmmo = CurrentEquippedWeapon->GetAmmo();
	int32 AmmoToReload = CurrentEquippedWeapon->GetMaxAmmo() - CurrentAmmo; // how much do we need
	int32 ReloadedAmmo = FMath::Min(AvailAmmo, AmmoToReload); // choose the min
	if (NumberOfAmmo > 0)
	{
		ReloadedAmmo = FMath::Min(ReloadedAmmo, NumberOfAmmo); // if we've requested less than is available - take this less
	}

	AmmunitionArray[(uint32)CurrentEquippedWeapon->GetAmmoType()] -= ReloadedAmmo;
	CurrentEquippedWeapon->SetAmmo(CurrentAmmo + ReloadedAmmo);

	if (bCheckIsFull)
	{
		AvailAmmo = AmmunitionArray[(uint32)CurrentEquippedWeapon->GetAmmoType()];
		bool bIsFullyReloaded = CurrentEquippedWeapon->GetAmmo() == CurrentEquippedWeapon->GetMaxAmmo();

		if (AvailAmmo == 0 || bIsFullyReloaded)
		{
			CurrentEquippedWeapon->EndReload(true);
		}
	}
}

void UCharacterEquipmentComponent::SwitchWeaponMode()
{
	if (!IsValid(CurrentEquippedWeapon))
	{
		return;
	}

	CurrentEquippedWeapon->ToggleAltMode();

	// CurrentEquippedWeapon - ARangeWeaponItem*
	// BP_Rifle < ARangeWeaponItem < AEquipableItem
	//		RangeWeaponItem.WeaponBarrel = WeaponBarrelComponent   -- modes and settings here
	//			EHitRegistrationType, FiringRange, BulletsPerShot, DamageAmount, TraceFX, DmgTypeClass, HitRegType, GrenadeClass
	//			Shot(), LaunchProjectile(), ProcessHit()

	// RangeWeaponItem
	//	class UWeaponBarrelComponent* WeaponBarrel;
	//	class UWeaponBarrelComponent* AltWeaponBarrel;
	// void ToggleAltMode()  alt_mode = !alt_mode;     <-- CharEquipComp.SwitchWeaponMode()  <-- V
	// private: bool alt_mode = false;
	//			GetBarrel() return alt_mode && IsValid(AltWeaponBarrel) ? AltWeaponBarrel : WeaponBarrel
	//     use GetBarrel() everywhere

	// RWI.MakeShot() - Shot(): GetBarrel()->Shot()

	// в общем, как будто просто должно быть 2 баррела и переключение между ними
	// а настраиваются они в блупринте
}

void UCharacterEquipmentComponent::OnWeaponReloadComplete()
{
	ReloadAmmoInCurrentWeapon();
}

void UCharacterEquipmentComponent::OnGrenadesAmmoChanged(int32 NewGrenades)
{
	if (IsValid(CurrentThrowable) && OnGrenadesAmmoChangedEvent.IsBound())
	{
		OnGrenadesAmmoChangedEvent.Broadcast(NewGrenades, CurrentThrowable->GetMaxAmmo());
	}
}

void UCharacterEquipmentComponent::OnRep_CurrentEquippedSlot(EEquipmentSlots Slot)
{
	EquipItemSlot(CurrentEquippedSlot);
}

void UCharacterEquipmentComponent::OnCurrentWeaponAmmoChanged(int32 NewAmmo)
{
	if (OnCurrentWeaponAmmoChangedEvent.IsBound())
	{
		ARangeWeaponItem* RangeWeapon = GetCurrentRangeWeapon();		
		if (IsValid(RangeWeapon))
		{
			OnCurrentWeaponAmmoChangedEvent.Broadcast(NewAmmo, AmmunitionArray[(uint32)RangeWeapon->GetAmmoType()]);
		}
	}
}

int32 UCharacterEquipmentComponent::GetAvailableAmmoForCurWeapon() const
{
	ARangeWeaponItem* RangeWeapon = GetCurrentRangeWeapon();
	if (IsValid(RangeWeapon))
	{
		return AmmunitionArray[(uint32)RangeWeapon->GetAmmoType()];
	}
	else {
		return 0; // TODO: do we have 0 throwables?
	}
}

AMeleeWeaponItem* UCharacterEquipmentComponent::GetCurrentMeleeWeapon() const
{
	return CurrentMeleeWeapon;
}

void UCharacterEquipmentComponent::ReloadCurrentWeapon()
{
	check(IsValid(CurrentEquippedWeapon));
	if (AmmunitionArray[(uint32)CurrentEquippedWeapon->GetAmmoType()] > 0)
	{
		CurrentEquippedWeapon->StartReload();
	}
}

void UCharacterEquipmentComponent::Srv_EquipItemInSlot_Implementation(EEquipmentSlots Slot)
{
	EquipItemSlot(Slot);
}

void UCharacterEquipmentComponent::EquipItemSlot(EEquipmentSlots Slot)
{
	if (IsEquipping()) return;
	if (!CachedChar.IsValid())
	{
		return;
	}

	UnEquipCurrentItem();

	if (!ItemsArray.IsValidIndex((uint32)Slot) || !IsValid(ItemsArray[(uint32)Slot])) return;

	CurrentEquippedItem = ItemsArray[(uint32)Slot];
	CurrentEquippedWeapon = Cast<ARangeWeaponItem>(CurrentEquippedItem);
	CurrentThrowable = Cast<AThrowableItem>(CurrentEquippedItem);
	CurrentMeleeWeapon = Cast<AMeleeWeaponItem>(CurrentEquippedItem);

	// equip new weapon
	if (IsValid(CurrentEquippedItem))
	{
		if (IsValid(CurrentThrowable))
		{
			if (!CurrentThrowable->CanThrow()) return;
			OnGrenadesChangeHnd = CurrentThrowable->OnAmmoChanged.AddUFunction(this, FName("OnGrenadesAmmoChanged"));
		}

		UAnimMontage* EquipMontage = CurrentEquippedItem->GetCharEquipMontage();
		if (IsValid(EquipMontage))
		{
			bIsEquipping = true;
			float Duration = CachedChar->PlayAnimMontage(EquipMontage);
			GetWorld()->GetTimerManager().SetTimer(EquipTimer, this, &UCharacterEquipmentComponent::EquipAnimFinished, Duration, false);
		}
		else {
			EquipAnimFinished();
		}

		CurrentEquippedItem->Equip();
		if (OnEquippedItemChangedEvent.IsBound()) OnEquippedItemChangedEvent.Broadcast(CurrentEquippedItem);

	} else if (IsValid(CurrentEquippedWeapon)) {

		OnAmmoChangeHnd = CurrentEquippedWeapon->OnAmmoChanged.AddUFunction(this, FName("OnCurrentWeaponAmmoChanged"));
		OnReloadHnd = CurrentEquippedWeapon->OnReloadComplete.AddUFunction(this, FName("OnWeaponReloadComplete"));
		// set current number of ammo to HUD
		OnCurrentWeaponAmmoChanged(CurrentEquippedWeapon->GetAmmo());
	}

	if (IsValid(CurrentEquippedItem) || IsValid(CurrentEquippedWeapon))
	{
		// character's owner call EquipItemSlot() on server
		if (CachedChar->GetLocalRole() == ROLE_AutonomousProxy)
		{
			Srv_EquipItemInSlot(Slot);			
		}

		CurrentEquippedSlot = Slot; // it replicates to all clients
	}
}

void UCharacterEquipmentComponent::AttachCurrentItem()
{
	if (IsValid(CurrentEquippedItem))
	{
		CurrentEquippedItem->AttachToComponent(
			CachedChar->GetMesh(),
			FAttachmentTransformRules::KeepRelativeTransform,
			CurrentEquippedItem->GetEquippedSocketName()
		);
	}
}

void UCharacterEquipmentComponent::UnEquipCurrentItem()
{
	// move equipped weapon to unequipped slot
	if (IsValid(CurrentEquippedItem))
	{
		CurrentEquippedItem->AttachToComponent(CachedChar->GetMesh(), 
			FAttachmentTransformRules::KeepRelativeTransform, 
			CurrentEquippedItem->GetUnEquippedSocketName());
		CurrentEquippedItem->UnEquip();
	}

	if (IsValid(CurrentEquippedWeapon))
	{
		CurrentEquippedWeapon->StopFire();
		CurrentEquippedWeapon->EndReload(false);
		CurrentEquippedWeapon->OnAmmoChanged.Remove(OnAmmoChangeHnd);
		//CurrentThrowable->OnAmmoChanged.Remove(OnGrenadesChangeHnd);
		CurrentEquippedWeapon->OnReloadComplete.Remove(OnReloadHnd);
		CurrentEquippedWeapon = nullptr;
	}

	PrevEquippedSlot = CurrentEquippedSlot;
	CurrentEquippedSlot = EEquipmentSlots::None;
}

void UCharacterEquipmentComponent::EquipNextItem()
{
	uint32 CurrentSlotIndex = (uint32)CurrentEquippedSlot;
	uint32 NextSlotIndex = NextItemsArraySlotIndex(CurrentSlotIndex);
	
	while (CurrentSlotIndex != NextSlotIndex 
		&& (!IsValid(ItemsArray[NextSlotIndex]) || IgnoreSlotsForSwitch.Contains((EEquipmentSlots) NextSlotIndex)))
	{
		NextSlotIndex = NextItemsArraySlotIndex(NextSlotIndex);
	}

	if (CurrentSlotIndex == NextSlotIndex) return;
	EquipItemSlot((EEquipmentSlots)NextSlotIndex);
}

//void UCharacterEquipmentComponent::EquipSideArm()
//{
//	EquipItemSlot(EEquipmentSlots::SideArm);
//}
//
//void UCharacterEquipmentComponent::EquipPrimaryWeapon()
//{
//	EquipItemSlot(EEquipmentSlots::PrimaryWeapon);
//}
//
//void UCharacterEquipmentComponent::EquipSecondaryWeapon()
//{
//	EquipItemSlot(EEquipmentSlots::SecondaryWeapon);
//}

void UCharacterEquipmentComponent::EquipPreviousItem()
{
	uint32 CurrentSlotIndex = (uint32)CurrentEquippedSlot;
	uint32 PrevSlotIndex = PrevItemsArraySlotIndex(CurrentSlotIndex);

	while (CurrentSlotIndex != PrevSlotIndex 
		&& (!IsValid(ItemsArray[PrevSlotIndex]) || IgnoreSlotsForSwitch.Contains((EEquipmentSlots)PrevSlotIndex))
	){
		PrevSlotIndex = PrevItemsArraySlotIndex(PrevSlotIndex);
	}

	if (CurrentSlotIndex == PrevSlotIndex) return;
	EquipItemSlot((EEquipmentSlots)PrevSlotIndex);
}

void UCharacterEquipmentComponent::LaunchCurrentThrowable()
{
	if (!CurrentThrowable)
		return;

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy || GetOwner()->HasAuthority())
	{
		CurrentThrowable->Throw();

		bIsEquipping = false;
		EquipItemSlot(PrevEquippedSlot);
	}
}
