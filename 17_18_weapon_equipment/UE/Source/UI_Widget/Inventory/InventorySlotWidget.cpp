#include "InventorySlotWidget.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "Inventory/Items/InventoryItem.h"
#include "Characters/BaseCharacter.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Utils/DeexDataTableUtils.h"
#include "Components/StaticMeshComponent.h"
#include "../ItemDragOp.h"
#include "Inventory/Items/Equipables/WeaponItem.h"

void UInventorySlotWidget::InitItemSlot(FInventorySlot& SlotInit)
{
	LinkedSlot = &SlotInit;

	FInventorySlot::FInventorySlotUpdate OnInventorySlotUpdate;
	OnInventorySlotUpdate.BindUObject(this, &UInventorySlotWidget::UpdateView);

	LinkedSlot->BindOnSlotUpdate(OnInventorySlotUpdate);
}

void UInventorySlotWidget::UpdateView()
{
	if (LinkedSlot == nullptr || !LinkedSlot->Item)
	{
		ImageItemIcon->SetBrushFromTexture(nullptr);
		SetCountText(0);
		return;
	}

	SetCountText(LinkedSlot->Count);
	const FInventoryItemDesc& Desc = LinkedSlot->Item->GetDescription();
	ImageItemIcon->SetBrushFromTexture(Desc.Icon);
}

void UInventorySlotWidget::SetItemIcon(UTexture2D* Icon)
{
	ImageItemIcon->SetBrushFromTexture(Icon);
}

void UInventorySlotWidget::SetCountText(int32 Count)
{
	CountText->SetText(FText::AsNumber(Count));
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (LinkedSlot == nullptr || !LinkedSlot->Item)
		return FReply::Handled();

	FKey MouseBtn = InMouseEvent.GetEffectingButton();
	if (MouseBtn == EKeys::RightMouseButton)
	{
		TWeakObjectPtr<UInventoryItem> Item = LinkedSlot->Item;
		ABaseCharacter* Owner = Cast<ABaseCharacter>(Item->GetOuter());

		if (Item->IsConsumable() && Item->Consume(Owner))
			LinkedSlot->ClearSlot();

		return FReply::Handled();
	}

	FEventReply Reply = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
	return Reply.NativeReply;
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UItemDragOp* DragOp = Cast<UItemDragOp>(UWidgetBlueprintLibrary::CreateDragDropOperation(UItemDragOp::StaticClass()));

	UInventorySlotWidget* DragWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), GetClass());
	DragWidget->ImageItemIcon->SetBrushFromTexture(LinkedSlot->Item->GetDescription().Icon);

	DragOp->DefaultDragVisual = DragWidget;
	DragOp->Pivot = EDragPivot::MouseDown;
	DragOp->Payload = LinkedSlot->Item;
	DragOp->Count = LinkedSlot->Count;
	
	OutOperation = DragOp;

	LinkedSlot->ClearSlot();
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragOp* DragOp = Cast<UItemDragOp>(InOperation);

	// slot is occupied
	if (LinkedSlot->Item || !DragOp)
		return false;

	UInventoryItem* Ptr = Cast<UInventoryItem>(DragOp->Payload);
	if (!Ptr)
		return false;

	LinkedSlot->Item = Ptr;
	LinkedSlot->Count = DragOp->Count;
	LinkedSlot->UpdateSlotState();
	return true;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragOp* DragOp = Cast<UItemDragOp>(InOperation);

	UInventoryItem* Ptr = Cast<UInventoryItem>(DragOp->Payload);
	if (!Ptr)
		return;

	FName ID = Ptr->GetID();

	FItemTableRow* Item = DeexDataTableUtils::FindInvItemData(ID);
	FAmmoTableRow* Ammo = DeexDataTableUtils::FindAmmoData(ID);
	FWeaponTableRow* Weapon = DeexDataTableUtils::FindWeaponData(ID);

	TSubclassOf<APickableItem> PickableActorClass = nullptr;

	UWeaponItem* WeaponPtr = Cast<UWeaponItem>(DragOp->Payload);

	if (Weapon && WeaponPtr)
		PickableActorClass = Weapon->PickableActor;
	else if (Item)
		PickableActorClass = Item->PickableActorClass;
	else if(Ammo)
		PickableActorClass = Ammo->PickableActor;	
	else {
		return;
	}

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
