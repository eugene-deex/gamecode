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

	ABaseCharacter* Char = GetCharOwner();
	if (!IsValid(Char))
	{
		return;
	}

	FVector ThrowLoc = FVector::ZeroVector;
	FRotator ThrowRot = FRotator::ZeroRotator;

	if (Char->IsPlayerControlled())
	{
		APlayerController* Ctrl = Char->GetController<APlayerController>();
		if (!IsValid(Ctrl)) return;

		Ctrl->GetPlayerViewPoint(ThrowLoc, ThrowRot);

	} else {
		ThrowLoc = Char->GetActorLocation();
		ThrowRot = Char->GetBaseAimRotation();
	}

	FVector ViewDir = ThrowRot.RotateVector(FVector::ForwardVector);
	FVector ViewUpVector = ThrowRot.RotateVector(FVector::UpVector);
	FVector LaunchDir = ViewDir + ViewUpVector * FMath::Tan(FMath::RadiansToDegrees(ThrowAngle)); // throw direction taking into account throw angle

	FVector ThrowSocketLoc = Char->GetMesh()->GetSocketLocation(SocketCharacterThrowable); // global coords
	FTransform ViewTransform(ThrowRot, ThrowLoc);
	FVector ViewSpaceSockCoords = ViewTransform.InverseTransformPosition(ThrowSocketLoc); // convert to user space
	FVector SpawnLoc = ThrowLoc + ViewDir * ViewSpaceSockCoords.X;

	AGCProjectile* Proj = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, SpawnLoc, ViewDir.ToOrientationRotator()); // FRotator::ZeroRotator
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
