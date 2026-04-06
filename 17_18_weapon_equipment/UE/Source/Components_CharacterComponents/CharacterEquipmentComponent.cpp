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
#include "Actors/Interactive/Pickables/PickableAmmo.h"
#include "Characters/Controllers/BasePlayerController.h"
#include "Inventory/Items/InventoryItem.h"
#include "Utils/DeexDataTableUtils.h"

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
	return CurrentEquippedItem.IsValid() ? CurrentEquippedItem->GetItemType() : EEquipableItemType::None;
}

void UCharacterEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("UCharacterAttributesComponent::BeginPlay can be used only with ABaseCharacter"))
	CachedChar = StaticCast<ABaseCharacter*>(GetOwner());

	CreateLoadout();
	AutoEquip();

	UE_LOG(LogTemp, Warning, TEXT("ViewWidgetClass: %s"), DeexUtils::LOG(ViewWidgetClass));

	// create equipment widget
	if (!ViewWidget)
	{
		ABasePlayerController* Ctrl = CachedChar->GetController<ABasePlayerController>();
		if(Ctrl && Ctrl->IsPlayerController())
			CreateViewWidget(Ctrl);
	}
	
	// fill WeaponAmmoTypes
	for (AEquipableItem* Item : ItemsArray)
	{
		if (!Item || !IsValid(Item)) 
			continue;

		TSubclassOf<AEquipableItem> Cls = Item->GetClass();

		ARangeWeaponItem* RangeWeapon = Cast<ARangeWeaponItem>(Item);
		AThrowableItem* Throwable = Cast<AThrowableItem>(Item);
		AMeleeWeaponItem* MeleeWeapon = Cast<AMeleeWeaponItem>(Item);

		if (IsValid(RangeWeapon))
			WeaponAmmoTypes.Add(Cls, RangeWeapon->GetAmmoType());
		else if (IsValid(Throwable))
			WeaponAmmoTypes.Add(Cls, EAmmunitionType::Grenades);
		else if (IsValid(MeleeWeapon))
			WeaponAmmoTypes.Add(Cls, EAmmunitionType::None);
	}
}

void UCharacterEquipmentComponent::CreateViewWidget(APlayerController* Ctrl)
{
	checkf(IsValid(ViewWidgetClass), TEXT("UCharacterEquipmentComponent::CreateViewWidget ViewWidgetClass is not set"));
	
	if (!IsValid(Ctrl))
		return;

	ViewWidget = CreateWidget<UEquipmentViewWidget>(Ctrl, ViewWidgetClass);
	ViewWidget->InitEquipmentWidget(this);
}

void UCharacterEquipmentComponent::OnRep_ItemsArray()
{
	for(AEquipableItem* Item : ItemsArray)
		if (IsValid(Item))
			Item->UnEquip();
}

void UCharacterEquipmentComponent::CreateLoadout()
{
	if (!CachedChar->HasAuthority() && CachedChar->GetLocalRole() != ROLE_AutonomousProxy)
		return;
	
	const bool isPlayerGetAmmonOnStart = false;

	ItemsArray.AddZeroed((uint32)EEquipmentSlots::MAX);

	// give the character weapons on start
	for (const TPair<EEquipmentSlots, TSubclassOf<AEquipableItem>>& ItemPair : ItemsLoadout)
	{

		AEquipableItem* Item = nullptr;
		if (!AddEquipmentItemToSlot(ItemPair.Value, (uint32)ItemPair.Key, Item))
			continue;

		// set grenade number to HUD
		//if (Item != nullptr)
		//{
		//	AThrowableItem* Throwable = Cast<AThrowableItem>(Item);
		//	if (IsValid(Throwable) && OnGrenadesAmmoChangedEvent.IsBound())
		//	{
		//		OnGrenadesAmmoChangedEvent.Broadcast(Throwable->GetAmmo(), Throwable->GetMaxAmmo());
		//		
		//		if (isPlayerGetAmmonOnStart) // give the player 1 grenade
		//		{
		//			const UEnum* EnumPtr = StaticEnum<EAmmunitionType>();
		//			if (EnumPtr)
		//			{
		//				FString ID = EnumPtr->GetNameStringByIndex((int32)EAmmunitionType::Grenades);
		//				APickableAmmo::CreateAndGiveAmmo(*ID, CachedChar.Get());
		//			}
		//		}
		//	}
		//}
	}

	// give the character ammo on start
	AmmunitionArray.AddZeroed((uint32)EAmmunitionType::MAX);

	if (isPlayerGetAmmonOnStart)
	{
		const UEnum* EnumPtr = StaticEnum<EAmmunitionType>();
		if (EnumPtr)
		{
			for (int32 idx = 0; idx < EnumPtr->NumEnums() - 1; ++idx)
			{
				FString ID = EnumPtr->GetNameStringByIndex(idx);
				APickableAmmo::CreateAndGiveAmmo(*ID, CachedChar.Get());
			}
		}
	}

	EquipNextItem();
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

void UCharacterEquipmentComponent::ReloadAmmoInCurrentWeapon(uint32 NumberOfAmmo /*= 0*/, bool bCheckIsFull /*= false*/)
{
	uint32 AvailAmmo = GetAvailableAmmoForCurWeapon(); // how much do we have
	if (AvailAmmo <= 0)
	{
		return;
	}

	uint32 CurrentAmmo = CurrentEquippedWeapon->GetAmmo();
	uint32 AmmoToReload = CurrentEquippedWeapon->GetMaxAmmo() - CurrentAmmo; // how much do we need
	uint32 ReloadedAmmo = FMath::Min(AvailAmmo, AmmoToReload); // choose the min
	if (NumberOfAmmo > 0)
	{
		ReloadedAmmo = FMath::Min(ReloadedAmmo, NumberOfAmmo); // if we've requested less than is available - take this less
	}

	int32 AmmoType = (int32) CurrentEquippedWeapon->GetAmmoType();
	AmmunitionArray[AmmoType] -= ReloadedAmmo;
	CurrentEquippedWeapon->SetAmmo(CurrentAmmo + ReloadedAmmo);

	FName WeaponID = CurrentEquippedWeapon->GetDataTableID();
	FWeaponTableRow* WeaponRow = DeexDataTableUtils::FindWeaponData(WeaponID);
	if (WeaponRow && WeaponRow->PickableActor)
	{
		if (OnAmmoRemovedEvent.IsBound())
			OnAmmoRemovedEvent.Broadcast(CurrentEquippedWeapon->GetClass(), ReloadedAmmo, GetCurrentAmmoByWeaponType(CurrentEquippedWeapon->GetClass()));

		if (UpdateInventoryEvent.IsBound())
			UpdateInventoryEvent.Broadcast(WeaponID, AmmoType, ReloadedAmmo, false);
	}

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
	SyncEquipmentWidget();
}

void UCharacterEquipmentComponent::OnGrenadesAmmoChanged(uint32 NewGrenades)
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

void UCharacterEquipmentComponent::OnCurrentWeaponAmmoChanged(uint32 NewAmmo)
{
	if (OnCurrentWeaponAmmoChangedEvent.IsBound())
	{
		ARangeWeaponItem* RangeWeapon = GetCurrentRangeWeapon();
		if (IsValid(RangeWeapon))
		{
			int32 AmmoType = (uint32)RangeWeapon->GetAmmoType() - 1;
			OnCurrentWeaponAmmoChangedEvent.Broadcast(NewAmmo, AmmunitionArray[AmmoType]);
		}
	}
}

uint32 UCharacterEquipmentComponent::GetAvailableAmmoForCurWeapon() const
{
// TODO: it returns first we have, not the active selected weapon
	ARangeWeaponItem* RangeWeapon = GetCurrentRangeWeapon();
	if (IsValid(RangeWeapon))
		return AmmunitionArray[(uint32)RangeWeapon->GetAmmoType()];
	
	if (IsValid(CurrentThrowable))
		return AmmunitionArray[(uint32)EEquipableItemType::Throwable];
	
	if (IsValid(CurrentMeleeWeapon))
		return AmmunitionArray[(uint32)EEquipableItemType::Melee];

	return -1;
}

uint32 UCharacterEquipmentComponent::GetCurrentAmmoByWeaponType(TSubclassOf<AEquipableItem> WeaponClass) const
{
	for (AEquipableItem* Item : ItemsArray)
	{
		if (Item && Item->GetClass() == WeaponClass.Get())
		{
			ARangeWeaponItem* RangeWeapon = Cast<ARangeWeaponItem>(Item);
			if (IsValid(RangeWeapon))
				return RangeWeapon->GetAmmo();

			AThrowableItem* Throwable = Cast<AThrowableItem>(Item);
			if (IsValid(Throwable))
				return Throwable->GetAmmo();

			AMeleeWeaponItem* MeleeWeapon = Cast<AMeleeWeaponItem>(Item);
			if (IsValid(MeleeWeapon))
				return 1;
		}
	}

	return -1;
}

EAmmunitionType UCharacterEquipmentComponent::GetAmmoTypeByWeaponType(TSubclassOf<AEquipableItem> WeaponClass) const
{
	if (!WeaponClass || !WeaponAmmoTypes.Contains(WeaponClass))
		return EAmmunitionType::None;

	return WeaponAmmoTypes[WeaponClass];
}

TSubclassOf<AEquipableItem> UCharacterEquipmentComponent::GetWeaponClassByAmmoType(EAmmunitionType AmmoType)
{
	for (AEquipableItem* Item : ItemsArray)
	{
		if(!Item)
			continue;

		ARangeWeaponItem* RangeWeapon = Cast<ARangeWeaponItem>(Item);
		if (IsValid(RangeWeapon))
		{
			EAmmunitionType WeaponAmmoType = RangeWeapon->GetAmmoType();
				
			if(WeaponAmmoType == AmmoType)
				return RangeWeapon->GetClass();
		}
	}
	
	return nullptr;
}

AMeleeWeaponItem* UCharacterEquipmentComponent::GetCurrentMeleeWeapon() const
{
	return CurrentMeleeWeapon;
}

bool UCharacterEquipmentComponent::AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem> EquipableItemClass, int32 SlotIndex, AEquipableItem*& SpawnedItem)
{
	SpawnedItem = nullptr;
	if (!IsValid(EquipableItemClass))
		return false;

	AEquipableItem* ItemObj = EquipableItemClass->GetDefaultObject<AEquipableItem>();
	
	// to not give the player 1 grenade on start
	//if (ItemObj->GetItemType() == EEquipableItemType::Throwable)
	//	return false;

	if (!ItemObj->IsSlotCompatible((EEquipmentSlots)SlotIndex))
		return false;

	if (!IsValid(ItemsArray[SlotIndex]))
	{
		AEquipableItem* Item = GetWorld()->SpawnActor<AEquipableItem>(EquipableItemClass);
		if (!Item)
			return false;

		if (Item->GetItemType() == EEquipableItemType::Melee) // TODO: it hides all melee weapons, not only knife
		{
			Item->SetActorHiddenInGame(true);
		}

		Item->AttachToComponent(CachedChar->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Item->GetUnEquippedSocketName());
		Item->SetOwner(CachedChar.Get());
		ItemsArray[SlotIndex] = Item;
		SpawnedItem = Item;
	}

	// TODO: AmmunitionArray depends on ammo in inventory
	//else if(ItemObj->IsA<ARangeWeaponItem>()) {

	//	ARangeWeaponItem* Weapon = StaticCast<ARangeWeaponItem*>(ItemObj); // Static because the type is 100% ARangeWeaponItem, faster than Cast
	//	int32 AmmoSlotIndex = (int32) Weapon->GetAmmoType();
	//	AmmunitionArray[SlotIndex] = Weapon->GetMaxAmmo();
	//}

	return true;
}

bool UCharacterEquipmentComponent::AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem> EquipableItemClass, int32 SlotIndex)
{
	AEquipableItem* p = nullptr;
	return AddEquipmentItemToSlot(EquipableItemClass, SlotIndex, p);
}

void UCharacterEquipmentComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	if ((uint32)CurrentEquippedSlot == SlotIndex)
		UnEquipCurrentItem();

	ItemsArray[SlotIndex]->Destroy();
	ItemsArray[SlotIndex] = nullptr;
}

void UCharacterEquipmentComponent::OpenViewEquipment(APlayerController* Ctrl)
{
	if (!ViewWidget)
		CreateViewWidget(Ctrl);


	if (ViewWidget && !IsViewVisible())
	{
		ViewWidget->UpdateCounters();
		ViewWidget->AddToViewport();
	}
}

bool UCharacterEquipmentComponent::IsViewVisible() const
{
	if (ViewWidget == nullptr)
		return false;

	if (ViewWidget->IsPendingKill() || !ViewWidget->IsValidLowLevel())
		return false;
	
	return ViewWidget->IsVisible();
}

void UCharacterEquipmentComponent::AddAmmo(EAmmunitionType AmmoType, uint32 Count, FName WeaponID)
{
	if ((uint32) AmmoType == 0u || (uint32) AmmoType == TNumericLimits<uint32>::Max() || !MaxAmunitionAmount.Contains(AmmoType))
		return;
	
	if (!AmmunitionArray.IsValidIndex((uint32)AmmoType))
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
			FString::Printf(TEXT("idi nahui: type %d"), (uint32)AmmoType));
		return;
	}

	int32 NewValue = AmmunitionArray[(uint32)AmmoType] + Count;
	AmmunitionArray[(uint32)AmmoType] = FMath::Clamp(NewValue, 0, MaxAmunitionAmount[AmmoType]);

	FName TypeName = StaticEnum<EAmmunitionType>()->GetNameByIndex((uint32)AmmoType);
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Purple, 
		FString::Printf(TEXT("AddAmmo: %d added to type %s, total %d"),
		Count, *TypeName.ToString(), NewValue));

	if (OnAmmoAddedEvent.IsBound())
	{
		TSubclassOf<AEquipableItem> WeaponClass = UCharacterEquipmentComponent::GetWeaponClassByAmmoType(AmmoType);
		if(WeaponClass)
			OnAmmoAddedEvent.Broadcast(WeaponClass, AmmunitionArray[(uint32)AmmoType], GetCurrentAmmoByWeaponType(WeaponClass));

		if (UpdateInventoryEvent.IsBound())
		{
			UpdateInventoryEvent.Broadcast(WeaponID, (int32) AmmoType, Count, true);
		}
	}
}

//void UCharacterEquipmentComponent::RemoveAmmo(EAmmunitionType AmmoType, uint32 Count, TSubclassOf<APickableItem> PickableActor)
//{
//	if (!AmmunitionArray.IsValidIndex((uint32)AmmoType))
//		return;
//
//	int32 NewValue = AmmunitionArray[(uint32)AmmoType] - Count;
//	AmmunitionArray[(uint32)AmmoType] = FMath::Clamp(NewValue, 0, MaxAmunitionAmount[AmmoType]);
//
//	FName TypeName = StaticEnum<EAmmunitionType>()->GetNameByIndex((uint32)AmmoType);
//	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Purple,
//		FString::Printf(TEXT("RemoveAmmo: %d removed from type %s, total %d"),
//			Count, *TypeName.ToString(), NewValue));
//
//	if (OnAmmoRemovedEvent.IsBound())
//	{
//		TSubclassOf<AEquipableItem> WeaponClass = UCharacterEquipmentComponent::GetWeaponClassByAmmoType(AmmoType);
//		if (WeaponClass)
//			OnAmmoRemovedEvent.Broadcast(WeaponClass, Count, GetCurrentAmmoByWeaponType(WeaponClass));
//
//		if (UpdateInventoryEvent.IsBound())
//			UpdateInventoryEvent.Broadcast(WeaponID, Count, false);
//	}
//}

void UCharacterEquipmentComponent::ReloadCurrentWeapon()
{
	check(IsValid(CurrentEquippedWeapon));
	uint32 Index = (uint32)CurrentEquippedWeapon->GetAmmoType();
	
	if (AmmunitionArray.IsValidIndex(Index) && AmmunitionArray[Index] > 0)
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

	if (!ItemsArray.IsValidIndex((uint32)Slot) || !IsValid(ItemsArray[(uint32)Slot])) 
		return;

	UnEquipCurrentItem();

	CurrentEquippedItem = ItemsArray[(uint32)Slot];
	CurrentEquippedItem->SetActorHiddenInGame(false);

	CurrentEquippedWeapon = Cast<ARangeWeaponItem>(CurrentEquippedItem);
	CurrentThrowable = Cast<AThrowableItem>(CurrentEquippedItem);
	CurrentMeleeWeapon = Cast<AMeleeWeaponItem>(CurrentEquippedItem);

	// equip new weapon
	if (CurrentEquippedItem.IsValid())
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
		if (OnEquippedItemChangedEvent.IsBound()) 
			OnEquippedItemChangedEvent.Broadcast(CurrentEquippedItem.Get());
	}
	
	if (IsValid(CurrentEquippedWeapon)) 
	{
		OnAmmoChangeHnd = CurrentEquippedWeapon->OnAmmoChanged.AddUFunction(this, FName("OnCurrentWeaponAmmoChanged"));
		OnReloadHnd = CurrentEquippedWeapon->OnReloadComplete.AddUFunction(this, FName("OnWeaponReloadComplete"));
		// set current number of ammo to HUD
		OnCurrentWeaponAmmoChanged(CurrentEquippedWeapon->GetAmmo());

		if (OnAmmoRemovedEvent.IsBound())
		{
			TSubclassOf<AEquipableItem> WeaponClass = UCharacterEquipmentComponent::GetWeaponClassByAmmoType(CurrentEquippedWeapon->GetAmmoType());
			if (WeaponClass)
				OnAmmoRemovedEvent.Broadcast(WeaponClass, CurrentEquippedWeapon->GetAmmo(), GetCurrentAmmoByWeaponType(WeaponClass));
		}
	}

	if (CurrentEquippedItem.IsValid() || IsValid(CurrentEquippedWeapon))
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
	if (CurrentEquippedItem.IsValid())
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
	if (!IsValid(this)) return;

	// move equipped weapon to unequipped slot
	if (CurrentEquippedItem.IsValid())
	{
		//UE_LOG(LogTemp, Log, TEXT("Attaching %s to %s at socket %s"), *GetName(), GetAttachParent() ? *GetAttachParent()->GetName() : TEXT("null"), *SocketName.ToString());

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
