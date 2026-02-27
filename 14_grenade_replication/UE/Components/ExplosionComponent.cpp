#include "ExplosionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Utils/DeexUtils.h"

void UExplosionComponent::Explode(AController* Ctrl)
{
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(),
		MaxDamage,
		MinDamage,
		GetComponentLocation(),
		InnerRadius,
		OuterRadius,
		DamageFalloff, // way to change from max dmg to min dmg on outerradius
		DamageTypeClass,
		TArray<AActor*>{ GetOwner() },
		GetOwner(), // dmg cause
		Ctrl,
		ECC_Visibility // channel
	);

	if (IsValid(ExplosionVFX))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionVFX, GetComponentLocation());
		//UE_LOG(LogDeex, Warning, TEXT("explosion for %d"), DeexUtils::GetPlayerId(GetOwner()));
		//DeexUtils::Sphere(GetWorld(), GetComponentLocation(), true, FColor::Green); // explosion point
	}

	if (OnExplosion.IsBound())
	{
		OnExplosion.Broadcast(GetOwner());
	}
}
