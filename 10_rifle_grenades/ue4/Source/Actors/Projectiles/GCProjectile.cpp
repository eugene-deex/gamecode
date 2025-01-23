#include "GCProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"

AGCProjectile::AGCProjectile()
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(5.f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(CollisionComp);

	MoveComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MoveComp"));	
}

// Dir is normalized vector (1,0,0)
void AGCProjectile::LaunchProjectile(FVector Dir)
{
	MoveComp->Velocity = Dir * MoveComp->InitialSpeed;
	CollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
	OnProjectileLaunched();
}

void AGCProjectile::OnProjectileLaunched()
{

}

void AGCProjectile::BeginPlay()
{
	Super::BeginPlay();
	MoveComp->InitialSpeed = Velocity;
	CollisionComp->OnComponentHit.AddDynamic(this, &AGCProjectile::OnComponentHit);
}

void AGCProjectile::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OnProjectileHit.IsBound())
	{
		OnProjectileHit.Broadcast(Hit, MoveComp->Velocity.GetSafeNormal());
	}
}
