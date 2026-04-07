#include "PickableAmmo.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/Items/InventoryItem.h"
#include "Utils/DeexDataTableUtils.h"
#include "Characters/BaseCharacter.h"
#include "Components/SceneComponent.h"
#include "Inventory/Items/Equipables/AmmoItem.h"


APickableAmmo::APickableAmmo()
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Scene);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Scene);
}

void APickableAmmo::Interact(ABaseCharacter* Char)
{
	if(CreateAndGiveAmmo(ID, Char))
		Destroy();
}

bool APickableAmmo::CreateAndGiveAmmo(FName ID_, ABaseCharacter* Char)
{
	FAmmoTableRow* Row = DeexDataTableUtils::FindAmmoData(ID_);
	if (Row == nullptr)
		return false;

	UAmmoItem* Ammo = NewObject<UAmmoItem>(Char);
	Ammo->Initialize(ID_, Row->ItemDesc);
	Ammo->SetAmmoType(Row->AmmunitionType);

	return Char->PickupItem(Ammo, Row->Count);
}
