#include "ThrowableItem.h"
#include "Characters/BaseCharacter.h"
#include "Actors/Projectiles/GCProjectile.h"

AThrowableItem::AThrowableItem()
{
	
}

void AThrowableItem::BeginPlay()
{
	Super::BeginPlay();
	SetAmmo(MaxAmmo);
}

void AThrowableItem::Throw()
{
	if (!CanThrow())
	{
		return;
	}

	checkf(GetOwner()->IsA<ABaseCharacter>(), TEXT("AThrowableItem::Throw() only BaseChar can be an owner of a throwable"));
	ABaseCharacter* Char = StaticCast<ABaseCharacter*>(GetOwner());

	APlayerController* Ctrl = Char->GetController<APlayerController>();
	if (!IsValid(Ctrl)) return;

	FVector PlayerLoc;
	FRotator PlayerRot;
	Ctrl->GetPlayerViewPoint(PlayerLoc, PlayerRot);

	FVector ViewDir = PlayerRot.RotateVector(FVector::ForwardVector);
	FVector ViewUpVector = PlayerRot.RotateVector(FVector::UpVector);
	FVector LaunchDir = ViewDir + ViewUpVector * FMath::Tan(FMath::RadiansToDegrees(ThrowAngle)); // throw direction taking into account throw angle

	FVector ThrowSocketLoc = Char->GetMesh()->GetSocketLocation(SocketCharacterThrowable); // global coords
	FTransform ViewTransform(PlayerRot, PlayerLoc);
	FVector ViewSpaceSockCoords = ViewTransform.InverseTransformPosition(ThrowSocketLoc); // convert to user space
	FVector SpawnLoc = PlayerLoc + ViewDir * ViewSpaceSockCoords.X;

	AGCProjectile* Proj = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, SpawnLoc, FRotator::ZeroRotator);
	if (IsValid(Proj))
	{
		Proj->SetOwner(GetOwner());
		Proj->LaunchProjectile(LaunchDir.GetSafeNormal());

		SetAmmo(FMath::Max(Ammo - 1, 0));
	}
}

void AThrowableItem::SetAmmo(int32 NewAmmo)
{
	Ammo = FMath::Clamp(NewAmmo, 0, MaxAmmo);

	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast(NewAmmo);
	}
}
