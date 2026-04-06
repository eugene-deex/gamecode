#include "EquipmentSlotWidget.h"
#include "Inventory/Items/Equipables/WeaponItem.h"
#include "Utils/DeexDataTableUtils.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/Widget/Inventory/InventorySlotWidget.h"
#include "Inventory/Items/InventoryItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Interactive/Pickables/PickableItem.h"
#include "Characters/BaseCharacter.h"
#include "GameFramework/PlayerController.h"

void UEquipmentSlotWidget::InitEquipmentSlot(TWeakObjectPtr<AEquipableItem> Equipment, int32 Index, TWeakObjectPtr<UCharacterEquipmentComponent> EquipComp_)
{
	if (!Equipment.IsValid())
		return;

	LinkedEquipableItem = Equipment.Get();
	SlotIndexInComp = Index;

	FName WeaponID = Equipment->GetDataTableID();
	FWeaponTableRow* Data = DeexDataTableUtils::FindWeaponData(WeaponID);
	if (Data == nullptr)
		return;

	AdapterLinkedWeaponItem = NewObject<UWeaponItem>(Equipment->GetOwner());
	AdapterLinkedWeaponItem->Initialize(Equipment->GetDataTableID(), Data->WeaponItemDesc);
	AdapterLinkedWeaponItem->SetEquipWeaponClass(Data->EquipableActor);

	EquipComp = EquipComp_;
}

void UEquipmentSlotWidget::UpdateCounter()
{
	if (!EquipComp.IsValid())
		return;

	if (!LinkedEquipableItem)
	{
		SetAmmoCount(0);
		SetCurrentAmmoCount(0);
		return;
	}

	EAmmunitionType AmmoType = EquipComp->GetAmmoTypeByWeaponType(LinkedEquipableItem->GetClass());

	if (AmmoType == EAmmunitionType::None)
	{
		SetAmmoCount(1);
		SetCurrentAmmoCount(1);

	} else {
		int32 Total = EquipComp->GetAmmo(AmmoType); 
		SetAmmoCount(Total);

		int32 Current = EquipComp->GetCurrentAmmoByWeaponType(LinkedEquipableItem->GetClass());
		SetCurrentAmmoCount(Current);
	}
}

void UEquipmentSlotWidget::UpdateView()
{
	if (LinkedEquipableItem && AdapterLinkedWeaponItem)
	{
		ImageWeaponIcon->SetBrushFromTexture(AdapterLinkedWeaponItem->GetDescription().Icon);
		TBWeaponName->SetText(FText::FromName(AdapterLinkedWeaponItem->GetDescription().Name));
	}else{
		ImageWeaponIcon->SetBrushFromTexture(nullptr);
		TBWeaponName->SetText(FText::FromName(NAME_None));
	}

	UpdateCounter();
}

FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!LinkedEquipableItem)
		return FReply::Handled();

	// grenade cannot be unequipped
	EEquipableItemType Type = LinkedEquipableItem->GetItemType();
	if(Type == EEquipableItemType::Throwable)
		return FReply::Handled();

	FEventReply Reply = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
	return Reply.NativeReply;
}

void UEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	checkf(DragAndDropWidgetClass.Get() != nullptr, TEXT("UInventorySlotWidget::NativeOnDragDetected DragAndDropWidgetClass is not set"));

	if (!AdapterLinkedWeaponItem)
		return;

	UItemDragOp* DragOp = Cast<UItemDragOp>(UWidgetBlueprintLibrary::CreateDragDropOperation(UItemDragOp::StaticClass()));

	UInventorySlotWidget* DragWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), DragAndDropWidgetClass);
	DragWidget->SetItemIcon(AdapterLinkedWeaponItem->GetDescription().Icon);

	DragOp->DefaultDragVisual = DragWidget;
	DragOp->Pivot = EDragPivot::CenterCenter;
	DragOp->Payload = AdapterLinkedWeaponItem;
	OutOperation = DragOp;

	LinkedEquipableItem = nullptr;
	OnEquipmentRemovedFromSlot.ExecuteIfBound(SlotIndexInComp);
	UpdateView();
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragOp* DragOp = Cast<UItemDragOp>(InOperation);

	// slot is occupied
	if (LinkedEquipableItem || !DragOp)
		return false;

	UWeaponItem* OpObj = Cast<UWeaponItem>(DragOp->Payload);
	if (!IsValid(OpObj))
		return false;

	AdapterLinkedWeaponItem = OpObj;

	return OnEquipmentDropInSlot.Execute(OpObj->GetEquipWeaponClass(), SlotIndexInComp);
}

void UEquipmentSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragOp* DragOp = Cast<UItemDragOp>(InOperation);

	if (!DragOp)
		return;

	UInventoryItem* Ptr = Cast<UInventoryItem>(DragOp->Payload);
	if (!Ptr)
		return;

	FName ID = Ptr->GetID();

	FWeaponTableRow* Weapon = DeexDataTableUtils::FindWeaponData(ID);
	if (!Weapon)
		return;

	TSubclassOf<APickableItem> PickableActorClass = Weapon->PickableActor;

	APlayerController* PC = GetOwningPlayer();
	ABaseCharacter* Char = PC ? Cast<ABaseCharacter>(PC->GetPawn()) : nullptr;
	if (!Char)
		return;

	UWorld* World = Char->GetWorld();
	if (!World)
		return;

	FVector Loc = Char->GetActorLocation() + Char->GetActorForwardVector() * 100.f + Char->GetActorRightVector() * 50.f;
	AActor* Spawned = World->SpawnActor<AActor>(PickableActorClass, Loc, FRotator::ZeroRotator);

	if (!Spawned)
		return;

	UStaticMeshComponent* MeshComp = Spawned->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp)
		MeshComp = Cast<UStaticMeshComponent>(Spawned->GetRootComponent());

	if (!MeshComp)
		return;

	FVector LaunchVelocity = Char->GetActorForwardVector() * 500.f + Char->GetActorUpVector() * 500.f;

	//MeshComp->SetPhysicsLinearVelocity(LaunchVelocity, true);
	MeshComp->AddImpulse(LaunchVelocity * MeshComp->GetMass());
}
