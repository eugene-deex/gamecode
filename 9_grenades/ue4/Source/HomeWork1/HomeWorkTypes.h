#pragma once

#define ECC_Climbing ECC_GameTraceChannel1
#define ECC_Wallrunnable ECC_GameTraceChannel2
#define ECC_Bullet ECC_GameTraceChannel4

#define ENABLE_DEBUG_DRAW true

const FName LedgeDetectorDebugAll = FName("LedgeDetectorDebugAll");
const FName LedgeDetectorDebugForward = FName("LedgeDetectorDebugForward");
const FName LedgeDetectorDebugDown = FName("LedgeDetectorDebugDown");
const FName LedgeDetectorDebugOverlap = FName("LedgeDetectorDebugOverlap");

const FName CollisionProfilePawn = FName("Pawn");
const FName CollisionProfileRagdoll = FName("Ragdoll");
const FName CollisionProfileClimbing = FName("Climbing");
const FName CollisionProfilePawnInteractionVolume = FName("PawnInteractionVolume");

const FName CollisionProfileWall = FName("Wall");

const FName FPSCameraSocket = FName("CameraSocket");
const FName DebugCategoryCharacterAttributes = FName("CharacterAttributes"); // use DebugDrawAttributes() in CharacterAttributesComponent
const FName DebugCategoryRangeWeapon = FName("RangeWeapon"); // ~: ToggleDebugCategory RangeWeapon true

const FName SkeletonHeadSocket = FName("headSocket");
const FName SocketWeaponMuzzle = FName("Muzzle_Socket");
const FName SocketWeaponForeGrip = FName("ForeGripSocket");

const FName FXParamTraceEnd = FName("TraceEnd");
const FName SocketCharacterWeapon = FName("CharWeaponSocket");
const FName SocketCharacterThrowable = FName("ThrowableSocket");

const FName SectionMontageReloadEnd = FName("ReloadEnd");

// PrimaryWeaponSocket - keep rifle
// PistolHolsterSocket - keep pistol

UENUM(BlueprintType)
enum class EEquipableItemType : uint8
{
	None,
	Pistol,
	Rifle,
	Throwable
};

UENUM(BlueprintType)
enum class EAmunitionType : uint8
{
	None,
	Pistol,
	Rifle,
	ShotgunShells,
	Grenades,
	RifleGrenades,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EEquipmentSlots: uint8
{
	None,
	SideArm,
	PrimaryWeapon,
	SecondaryWeapon,
	PrimaryItem,
	ShotgunWeapon,
	MAX UMETA(Hidden)
};	

UENUM(BlueprintType)
enum class EReticleType : uint8
{
	None,
	Default,
	SniperRifle,
	MAX UMETA(Hidden)
};	