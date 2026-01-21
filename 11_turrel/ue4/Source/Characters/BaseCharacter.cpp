
#include "BaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PhysicsVolume.h"

#include "Actors/InteractiveActor.h"
#include "Actors/Ladder.h"
#include "Actors/Zipline.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"

#include "Components/Movement/BaseMovementComponent.h"
#include "Components/Movement/LedgeDetectorComponent.h"
#include "Components/CharacterComponents/CharacterAttributesComponent.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"

#include "Animation/AnimMontage.h"
#include "Curves/CurveVector.h"
#include "DrawDebugHelpers.h"
#include "HomeWorkTypes.h"
#include "HomeWork1.h"
#include "Actors/Equipment/Weapons/MeleeWeaponItem.h"
#include "AIController.h"


ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBaseMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	BaseCharacterMovementComponent = StaticCast<UBaseMovementComponent*>(GetCharacterMovement());

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;

	LedgeDetectorComponent = CreateDefaultSubobject<ULedgeDetectorComponent>(TEXT("Ledge detector")); 
	// no need to attach component to the root

	CharacterAttributesComponent = CreateDefaultSubobject<UCharacterAttributesComponent>(TEXT("CharacterAttributes"));

	EquipmentComponent = CreateDefaultSubobject<UCharacterEquipmentComponent>(TEXT("Equipment"));

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ABaseCharacter::OnOverlapBegin);


	GetMesh()->CastShadow = true;
	GetMesh()->bCastDynamicShadow = true;
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultArmLength = SpringArmComponent->TargetArmLength; // save original arm length

	if (LeftIKSocketName == "" || RightIKSocketName == "")
	{
		UE_CLOG(bDebug, LogDeex, Warning, TEXT("BaseCharacter::BeginPlay LeftIKSocketName/RightIKSocketName is not set"));
	}

	if (IsValid(TimelineCurve))
	{
		SprintArmTimeline.SetPlayRate(PlayRate);

		FOnTimelineFloatStatic SprintTimelineUpdate;
		SprintTimelineUpdate.BindUObject(this, &ABaseCharacter::SprintArmUpdate);
		SprintArmTimeline.AddInterpFloat(TimelineCurve, SprintTimelineUpdate);
	}

	CharacterAttributesComponent->OnDeathEvent.AddUObject(this, &ABaseCharacter::OnDeath);
	CharacterAttributesComponent->OutOfStaminaEvent.AddUObject(this, &ABaseCharacter::SetOutOfStamina);
}

void ABaseCharacter::PossessedBy(AController* Ctrl)
{
	Super::PossessedBy(Ctrl);

	AAIController* AICtrl = Cast<AAIController>(Ctrl);
	if (IsValid(AICtrl))
	{
		FGenericTeamId TeamId((uint8)Team);
		AICtrl->SetGenericTeamId(TeamId);
	}
}

void ABaseCharacter::Prone2Crouch()
{
	UE_CLOG(bDebug, LogDeex, Warning, TEXT("ABaseCharacter::Prone2Crouch - from prone to crouch"));

	// check if we can crouch here
	const FVector PawnLocation = GetActorLocation(); // GetCharacterMovement()->UpdatedComponent->GetComponentLocation();
	const float SweepInflation = KINDA_SMALL_NUMBER * 10.f;
	const float MinFloorDistanceHalf = (UCharacterMovementComponent::MIN_FLOOR_DIST / 2.f); // 1.9f/2.0

	const float FloorZ = PawnLocation.Z - BaseCharacterMovementComponent->GetPronedHalfHeightScaled() + BaseCharacterMovementComponent->GetCrouchedHalfHeightScaled() + SweepInflation + MinFloorDistanceHalf;
	const FVector Start = FVector(PawnLocation.X, PawnLocation.Y, FloorZ);
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	FHitResult OutHit;
	
	if (UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Start, Start, BaseCharacterMovementComponent->GetCapsuleRadiusScaled(), BaseCharacterMovementComponent->GetCrouchedHalfHeightScaled(), TraceType, false, TArray<AActor*>(), EDrawDebugTrace::ForDuration, OutHit, true, FLinearColor::Red, FLinearColor::Blue, 5.f))
	{
		UE_CLOG(bDebug, LogDeex, Warning, TEXT("Can't crouch here"));
		return;
	}

	bIsCrouched = true;
	GetMoveComp()->SetCurrentPlayerState(EPlayerState::Crouch);
	
	float Scale = GetActorScale3D().Z;
	float CrouchedHalfHeight = GetMoveComp()->GetCrouchedHalfHeightScaled(); // 60
	float PronedHalfHeight = GetMoveComp()->GetPronedHalfHeightScaled(); // 40

	float Adjust = CrouchedHalfHeight - PronedHalfHeight; // 60-40 = 20

	OnEndProne(Adjust / Scale, Adjust);
	GetCapsuleComponent()->SetCapsuleSize(GetMoveComp()->GetCapsuleRadiusScaled(), CrouchedHalfHeight);
}

void ABaseCharacter::Prone()
{
	UE_CLOG(bDebug, LogDeex, Warning, TEXT("ABaseCharacter::Prone - start prone"));

	float Scale = GetActorScale3D().Z;
	float Radius = GetMoveComp()->GetProneCapsuleRadiusScaled();
	float StayedHalfHeight = GetMoveComp()->GetStayedHalfHeight(); // 90
	float PronedHalfHeight = GetMoveComp()->GetPronedHalfHeightScaled(); // 40
	GetCapsuleComponent()->SetCapsuleSize(Radius, PronedHalfHeight, true);

	float Adjust = StayedHalfHeight - PronedHalfHeight;

	// move component down
	USceneComponent* Scene = GetMoveComp()->UpdatedComponent;
	Scene->MoveComponent(FVector(0.f, 0.f, -Adjust), Scene->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
	OnStartProne(Adjust / Scale, Adjust);
}

void ABaseCharacter::StartSprint()
{
	if (BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Prone)
	{
		return;
	}

	bIsSprintRequested = true;

	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
}

void ABaseCharacter::StopSprint()
{
	bIsSprintRequested = false;
}

void ABaseCharacter::StartFire()
{

	if (EquipmentComponent->IsEquipping())
	{
		return;
	}

	if (!CharacterAttributesComponent->IsAlive())
	{
		return;
	}

	ARangeWeaponItem* CurrentRangeWeapon = EquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StartFire();
	}
}

void ABaseCharacter::StopFire()
{
	ARangeWeaponItem* CurrentRangeWeapon = EquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopFire();
	}
}

void ABaseCharacter::StartAiming()
{
	if (!CharacterAttributesComponent->IsAlive())
	{
		return;
	}

	bIsAiming = true;
	ARangeWeaponItem* CurrentRangeWeapon = GetEquipmentComp()->GetCurrentRangeWeapon();
	if (!IsValid(CurrentRangeWeapon))
	{
		return;
	}

	bIsAiming = true;
	CurrentAimingMovementSpeed = CurrentRangeWeapon->GetAimMovementMaxSpeed();
	CurrentRangeWeapon->StartAim();
	OnStartAiming();
}

void ABaseCharacter::StopAiming()
{
	if (!bIsAiming)
	{
		return;
	}

	ARangeWeaponItem* CurrentRangeWeapon = GetEquipmentComp()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopAim();
	}

	bIsAiming = false;
	CurrentAimingMovementSpeed = 0.f;
	OnStopAiming();
}

void ABaseCharacter::Reload()
{
	if (IsValid(EquipmentComponent->GetCurrentRangeWeapon()))
	{
		EquipmentComponent->ReloadCurrentWeapon();
	}
}

float ABaseCharacter::GetAimingMovementSpeed() const
{
	return CurrentAimingMovementSpeed;
}

void ABaseCharacter::OnStartAiming_Implementation()
{
	OnStartAimingInternal();
}

void ABaseCharacter::OnStopAiming_Implementation()
{
	OnStopAimingInternal();
}

void ABaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TryChangeSprintState();

	GetMoveComp()->UpdateWallRun(DeltaSeconds);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	GEngine->AddOnScreenDebugMessage(3, 1.0f, FColor::Green, FString::Printf(TEXT("State: %s | max speed %f | is out of s: %s"), 
		*UEnum::GetDisplayValueAsText(BaseCharacterMovementComponent->GetCurrentState()).ToString(),
		BaseCharacterMovementComponent->GetMaxSpeed(),
		IsOutOfStamina()? TEXT("yes") : TEXT("no")
		));
#endif 

	// Replaced with HUD
	//if (CurrentStamina < MaxStamina)
	//{
	//	FColor Color = FColor::Yellow;
	//	float percent = MaxStamina / 100.f;
	//	if (CurrentStamina / percent < 30)
	//	{
	//		Color = FColor::Red;
	//	}

	//	GEngine->AddOnScreenDebugMessage(1, 1.0f, Color, FString::Printf(TEXT("Stamina: %.2f"), CurrentStamina));
	//}

	SprintArmTimeline.TickTimeline(DeltaSeconds);
	
	if (BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Stay && GetCharacterMovement()->Velocity.IsNearlyZero())
	{
		float left = CountOffset(LeftIKSocketName) / GetActorScale3D().Z;
		float right = CountOffset(RightIKSocketName) / GetActorScale3D().Z;
		float pelvis = 0.f;

		if (left < right) // lower pelvis on offset of the lowest foot
		{
			pelvis = left;
		}
		else {
			pelvis = right;
		}

		if (pelvis > 0.f) // we don't need to raise pelvis up
		{
			pelvis = 0.f;
		}

		left -= pelvis;
		right -= pelvis;

		LeftIKOffset = FMath::FInterpTo(LeftIKOffset, left, DeltaSeconds, IKInterpSpeed);
		RightIKOffset = FMath::FInterpTo(RightIKOffset, right, DeltaSeconds, IKInterpSpeed);
		PelvisIKOffset = FMath::FInterpTo(PelvisIKOffset, pelvis, DeltaSeconds, IKInterpSpeed);
	}
}

void ABaseCharacter::Mantle(bool bForce/* = false */)
{

	if ((!CanMantle() && !bForce) || BaseCharacterMovementComponent->IsMantling())
	{
		return;
	}

	if (!IsValid(HighMantleCfg.Curve))
	{
		UE_CLOG(bDebug, LogDeex, Error, TEXT("ABaseCharacter::Mantle() High Curve is not set"));
		return;
	}

	if (!IsValid(HighMantleCfg.Montage))
	{
		UE_CLOG(bDebug, LogDeex, Error, TEXT("ABaseCharacter::Mantle() High Montage is not set"));
		return;
	}

	if (!IsValid(LowMantleCfg.Curve))
	{
		UE_CLOG(bDebug, LogDeex, Error, TEXT("ABaseCharacter::Mantle() Low Curve is not set"));
		return;
	}

	if (!IsValid(LowMantleCfg.Montage))
	{
		UE_CLOG(bDebug, LogDeex, Error, TEXT("ABaseCharacter::Mantle() Low Montage is not set"));
		return;
	}

	FLedgeDescription Descr;
	if (LedgeDetectorComponent->DetectLedge(Descr))
	{
		FMantlingParameters Params;
		Params.ActorInitLoc = Descr.ActorLocation;
		Params.LedgeActor = Descr.LedgeActor;
		Params.InitLoc = GetActorLocation();
		Params.InitRot = GetActorRotation();
		Params.TargetLoc = Descr.Location;
		Params.TargetRot = Descr.Rotation;
		
		float Height = (Params.TargetLoc - Params.InitLoc).Z;
		const FMantlingSettings MantleCfg = GetMantlingSettings(Height); // low or high settings

		float MinRange;
		float MaxRange;
		MantleCfg.Curve->GetTimeRange(MinRange, MaxRange); // получить продолжительность кривой в переменные min/maxrange
		
		Params.Duration = MaxRange - MinRange; // узнаем длительность кривой

		// получаем время начала монтажа

		// Mathematician way:
		// float StartTime = MantleCfg.MaxHeightStartTime + (Height - MantleCfg.MinHeight) / (MantleCfg.MaxHeight - MantleCfg.MinHeight) * (MantleCfg.MaxHeightStartTime - MantleCfg.MinHeightStartTime);

		Params.Curve = MantleCfg.Curve;
		// realHeight = targetHeight - currentHeight
		// minHeight------realHeight-------------------------maxHeight
		// minTime(0.5)--------*-----------------------------maxTime(0)
		FVector2D SourceRange(MantleCfg.MinHeight, MantleCfg.MaxHeight); // диапазон
		FVector2D TargetRange(MantleCfg.MinHeightStartTime, MantleCfg.MaxHeightStartTime);

		// For the given Value clamped to the [Input:Range] inclusive, returns the corresponding percentage in [Output:Range] Inclusive
		Params.StartTime = FMath::GetMappedRangeValueClamped(SourceRange, TargetRange, Height);

		Params.InitAnimLoc = Params.TargetLoc - MantleCfg.AnimCorrectZ * FVector::UpVector + MantleCfg.AnimCorrectXY * Descr.LedgeNormal; // move 200 down and 65 to player

		UE_CLOG(bDebug, LogDeex, Warning, TEXT("Start mantling here"));
		BaseCharacterMovementComponent->StartMantle(Params);
		//		PlayAnimMontage(MantleCfg.Montage); - play full montage

		UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
		AnimInst->Montage_Play(MantleCfg.Montage, 1.f, EMontagePlayReturnType::Duration, Params.StartTime);

		// for FPS mantling
		OnMantle(MantleCfg, Params.StartTime);
	}
}

bool ABaseCharacter::CanMantle() const
{
	return !BaseCharacterMovementComponent->IsOnLadder() && !BaseCharacterMovementComponent->IsOnZipline();
}

void ABaseCharacter::StartSlide()
{
	UE_CLOG(bDebug, LogDeex, Warning, TEXT("Start slide"));

	if (!IsValid(SlideMontage))
	{
		UE_CLOG(bDebug, LogDeex, Error, TEXT("ABaseCharacter::StartSlide() Slide Montage is not set"));
		return;
	}

	GetMoveComp()->StartSlide(SlideMontage);
}

void ABaseCharacter::RegisterInteractiveActor(AInteractiveActor* Actor)
{
	AvailableInteractiveActors.AddUnique(Actor);
}

void ABaseCharacter::UnRegisterInteractiveActor(AInteractiveActor* Actor)
{
	AvailableInteractiveActors.RemoveSingleSwap(Actor);
}

float ABaseCharacter::CountOffset(FName SocketName) 
{
	float Result = 0.f;
	
	FVector SocketLocation = GetMesh()->GetSocketLocation(SocketName);
	FVector Start = FVector(SocketLocation.X, SocketLocation.Y, GetActorLocation().Z);
	FVector End = Start - (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + IKTraceDistance) * FVector::UpVector;
	
	FHitResult HitResult;
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	TArray<AActor*> Ignore = TArray<AActor*>();

	if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, TraceType, true, Ignore, EDrawDebugTrace::ForOneFrame, HitResult, true, FLinearColor::Green, FLinearColor::Black, 0.5f))
	{

		Result = (HitResult.Location.Z - End.Z) - IKTraceDistance;

		// TODO: Rotate feet
		//float ActorYaw = GetActorRotation().Yaw;
		//FootRotationDegree = HitResult.ImpactNormal.Rotation();
		//FootRotationDegree.Yaw = ActorYaw;
	}

	return Result;
}

void ABaseCharacter::ClimbLadderUp(float Value)
{
	if (GetMoveComp()->IsOnLadder() && !FMath::IsNearlyZero(Value))
	{
		FVector LadderUpVector = GetMoveComp()->GetCurrentLadder()->GetActorUpVector();
		AddMovementInput(LadderUpVector, Value);
	}
}

void ABaseCharacter::InteractZipline()
{
	if (GetMoveComp()->IsSliding())
	{
		return;
	}

	if (GetMoveComp()->IsOnZipline())
	{
		GetMoveComp()->DetachFromZipline();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(LadderTimerHandle, this, &ABaseCharacter::AttachZipline, 0.5f, false);
	}
}

void ABaseCharacter::AttachZipline()
{
	const AZipline* Zip = GetAvailableZipline();
	if (IsValid(Zip))
	{
		GetMoveComp()->AttachToZipline(Zip);
	}
}

void ABaseCharacter::SetSpringArmOffsetZ(float OffsetZ)
{
	SpringArmComponent->TargetOffset.Z = OffsetZ;
}

void ABaseCharacter::InteractLadder()
{
	if (GetMoveComp()->IsSliding())
	{
		return;
	}

	if (GetMoveComp()->IsOnLadder())
	{
		GetMoveComp()->DetachFromLadder(EDetachLadderMethod::JumpOff);
		CurrentLadder = nullptr;
	} else {
		const ALadder* Ladder = GetAvailableLadder();
		if (IsValid(Ladder))
		{
			if (Ladder->GetIsOnTop()) // player inside top attach volume
			{
				CurrentLadder = Ladder;

				FRotator TargetOrientRot = CurrentLadder->GetActorForwardVector().ToOrientationRotator();
				TargetOrientRot.Yaw += 180;
				SetActorRotation(TargetOrientRot);

				float Dur = PlayAnimMontage(Ladder->GetAttachFromTopAnimMontage()); // play anim attach to ladder
				GetWorld()->GetTimerManager().SetTimer(LadderTimerHandle, this, &ABaseCharacter::EndTopLadderAttach, Dur, false);
				return;
			}
			GetMoveComp()->AttachToLadder(Ladder); // rotate playa, move to point, set mov.mode Ladder
		}
	}
}

void ABaseCharacter::EndTopLadderAttach()
{
	GetMoveComp()->AttachToLadder(CurrentLadder);
}

void ABaseCharacter::EnableRagdoll()
{
	GetMesh()->SetCollisionProfileName(CollisionProfileRagdoll);
	GetMesh()->SetSimulatePhysics(true);
}

const class ALadder* ABaseCharacter::GetAvailableLadder() const
{
	for (const AInteractiveActor* Actor : AvailableInteractiveActors)
	{
		if (Actor->IsA<ALadder>())
		{
			return StaticCast<const ALadder*>(Actor);
		}
	}

	return nullptr;
}

const class AZipline* ABaseCharacter::GetAvailableZipline() const
{
	for (const AInteractiveActor* Actor : AvailableInteractiveActors)
	{
		if (Actor->IsA<AZipline>())
		{
			return StaticCast<const AZipline*>(Actor);
		}
	}

	return nullptr;
}

void ABaseCharacter::OnStartAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(true);
	}
}

void ABaseCharacter::OnStopAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(false);
	}
}

void ABaseCharacter::OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust)
{
	Super::OnStartCrouch(HeightAdjust, ScaledHeightAdjust);
	GetMoveComp()->SetCurrentPlayerState(EPlayerState::Crouch);
}

void ABaseCharacter::OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust)
{
	Super::OnEndCrouch(HeightAdjust, ScaledHeightAdjust);
	GetMoveComp()->SetCurrentPlayerState(EPlayerState::Stay);
}

void ABaseCharacter::OnSprintStarted_Implementation()
{
	SprintArmTimeline.Play();
}

void ABaseCharacter::OnSprintStopped_Implementation()
{
	SprintArmTimeline.Reverse();
}

bool ABaseCharacter::CanSprint()
{
	if (IsOutOfStamina() || BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Prone)
	{
		return false;
	}

	return true;
}

bool ABaseCharacter::CanCrouch() const
{
	if (BaseCharacterMovementComponent->IsSprinting())
	{
		return false;
	}

	return Super::CanCrouch();
}

void ABaseCharacter::OnOverlapBegin(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetMoveComp()->IsSliding())
	{
		GetMoveComp()->StopSlide();
	}

	if (GetMoveComp()->IsOnZipline())
	{
		GetMoveComp()->DetachFromZipline();
	}

	GetMoveComp()->StartWallrunOnHit(OtherActor, Hit.ImpactNormal);
}

void ABaseCharacter::OnMantle(const FMantlingSettings& MantlingSettings, float AnimStartTime)
{

}

void ABaseCharacter::TryChangeSprintState()
{
	if (bIsSprintRequested && !BaseCharacterMovementComponent->IsSprinting() && CanSprint())
	{
		BaseCharacterMovementComponent->StartSprint();
		OnSprintStarted();
	}

	if (!bIsSprintRequested && BaseCharacterMovementComponent->IsSprinting())
	{
		BaseCharacterMovementComponent->StopSprint();
		OnSprintStopped();
	}
}

void ABaseCharacter::SprintArmUpdate(const float Alpha)
{
	SpringArmComponent->TargetArmLength = FMath::Lerp(DefaultArmLength, SprintArmLength, Alpha);
}

bool ABaseCharacter::CanJumpInternal_Implementation() const
{
	if (IsOutOfStamina() || BaseCharacterMovementComponent->IsMantling() || IsValid(CurrentLadder))
	{
		return false;
	}

	if (BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Stay)
	{
		return Super::CanJumpInternal_Implementation();
	}
	
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	FHitResult OutHit;
	const FVector PawnLocation = GetActorLocation(); // Should I use GetCharacterMovement()->UpdatedComponent->GetComponentLocation();  ?
	const float SweepInflation = KINDA_SMALL_NUMBER * 10.f;
	const float MinFloorDistanceHalf = (UCharacterMovementComponent::MIN_FLOOR_DIST / 2.f); // 1.9f/2.0
	
	float StartZ = PawnLocation.Z + SweepInflation + MinFloorDistanceHalf + BaseCharacterMovementComponent->GetStayedHalfHeight();
	if(BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Crouch) 
	{
		StartZ -= BaseCharacterMovementComponent->GetCrouchedHalfHeightScaled();
	}
	else // prone state
	{
		StartZ -= BaseCharacterMovementComponent->GetPronedHalfHeightScaled();
	}

	const FVector Position = FVector(PawnLocation.X, PawnLocation.Y, StartZ);
	if(UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Position, Position, BaseCharacterMovementComponent->GetCapsuleRadiusScaled(), BaseCharacterMovementComponent->GetStayedHalfHeight(), TraceType, false, TArray<AActor*>(), EDrawDebugTrace::ForDuration, OutHit, true, FLinearColor::Red, FLinearColor::Blue, 5.f))
	{
		return false;
	}
	else {
		return true;
	}

	// Default behaviour - return Super::CanJumpInternal_Implementation();
}

void ABaseCharacter::OnJumped_Implementation()
{
	if (BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Prone)
	{
		Prone2Crouch();
		UnCrouch();
		if (!bIsCrouched)
		{
			GetMoveComp()->SetCurrentPlayerState(EPlayerState::Stay);
		}
	}
	else if (BaseCharacterMovementComponent->GetCurrentState() == EPlayerState::Crouch)
	{
		UnCrouch();
		if (!bIsCrouched)
		{
			GetMoveComp()->SetCurrentPlayerState(EPlayerState::Stay);
		}
	}

	Super::OnJumped_Implementation();
}

void ABaseCharacter::OnStartProne(float HeightAdjust, float ScaledHeightAdjust)
{
	RecalculateBaseEyeHeight();

	const ACharacter* DefaultChar = GetDefault<ACharacter>(GetClass());
	if (GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = DefaultChar->GetMesh()->GetRelativeLocation().Z + HeightAdjust;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = DefaultChar->GetBaseTranslationOffset().Z + HeightAdjust;
	}

	K2_OnStartCrouch(HeightAdjust, ScaledHeightAdjust);
	GetMoveComp()->SetCurrentPlayerState(EPlayerState::Prone);
}

void ABaseCharacter::OnEndProne(float HeightAdjust, float ScaledHeightAdjust)
{
	RecalculateBaseEyeHeight();

	const ACharacter* DefaultChar = GetDefault<ACharacter>(GetClass());
	if (GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = DefaultChar->GetMesh()->GetRelativeLocation().Z + HeightAdjust;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = DefaultChar->GetBaseTranslationOffset().Z + HeightAdjust;
	}
}

void ABaseCharacter::Falling()
{
//	Super::Falling();
	GetMoveComp()->bNotifyApex = true; // activate NotifyJumpApex

	CurrentFallApex = GetActorLocation();
}

void ABaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	float FallHeight = (CurrentFallApex - GetActorLocation()).Z / 100.f; // cm to meters

	if (IsValid(FallDamageCurve))
	{
		float DamageAmount = FallDamageCurve->GetFloatValue(FallHeight);
		TakeDamage(DamageAmount, FDamageEvent(), GetController(), Hit.GetActor()); // Hit.Actor.Get()
	}
}

void ABaseCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	topReachedZ = GetActorLocation().Z;
}

bool ABaseCharacter::IsAnyMontagePlaying() const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	return (IsValid(AnimInstance) && AnimInstance->IsAnyMontagePlaying());
}

void ABaseCharacter::OnDeath(bool bIsSwimming)
{
	GetCharacterMovement()->DisableMovement();

	if (bIsSwimming)
	{
		EnableRagdoll();
		return;
	}

	float Duration = PlayAnimMontage(OnDeathAnimMontage); // get montage time
	if (FMath::IsNearlyZero(Duration))
	{
		EnableRagdoll();
	}

	/*
	if (Duration > 0.f)
	{
		// enable ragdoll after the montage
		GetWorld()->GetTimerManager().SetTimer(DeathMontageTimer, this, &ABaseCharacter::EnableRagdoll, Duration, false);
	} else {
		EnableRagdoll();
	}
	*/
}

void ABaseCharacter::SetOutOfStamina(bool Value)
{
	bIsOutOfStamina = Value;
}

bool ABaseCharacter::IsSwimmingUnderWater() const
{
	if (!GetCharacterMovement()->IsSwimming())
	{
		return false;
	}

	APhysicsVolume* Volume = GetCharacterMovement()->GetPhysicsVolume();
	// volume location Z + half-height * scale
	float VolumeTopPlane = Volume->GetActorLocation().Z + Volume->GetBounds().BoxExtent.Z; // * Volume->GetActorScale3D().Z; - gives wrong height

	FVector HeadPosition = GetMesh()->GetSocketLocation(SkeletonHeadSocket);

	return VolumeTopPlane > HeadPosition.Z;
}

void ABaseCharacter::NextItem()
{
	EquipmentComponent->EquipNextItem();
}

void ABaseCharacter::PrevItem()
{
	EquipmentComponent->EquipPreviousItem();
}

void ABaseCharacter::EquipSideArm()
{
	EquipmentComponent->EquipItemSlot(EEquipmentSlots::SideArm);
}

void ABaseCharacter::EquipPrimaryWeapon()
{
	EquipmentComponent->EquipItemSlot(EEquipmentSlots::PrimaryWeapon);
}

void ABaseCharacter::EquipSecondaryWeapon()
{
	EquipmentComponent->EquipItemSlot(EEquipmentSlots::SecondaryWeapon);
}

void ABaseCharacter::EquipShotgunWeapon()
{
	EquipmentComponent->EquipItemSlot(EEquipmentSlots::ShotgunWeapon);
}

void ABaseCharacter::EquipPrimaryItem()
{
	EquipmentComponent->EquipItemSlot(EEquipmentSlots::PrimaryItem);
}

void ABaseCharacter::Unequip()
{
	EquipmentComponent->EquipItemSlot(EEquipmentSlots::None);
}

void ABaseCharacter::SwitchWeaponMode()
{
	EquipmentComponent->SwitchWeaponMode();
}

void ABaseCharacter::PrimaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = EquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackTypes::Primary);
	}
}

void ABaseCharacter::SecondaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = EquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackTypes::Secondary);
	}
}

FGenericTeamId ABaseCharacter::GetGenericTeamId() const
{
	return FGenericTeamId((uint8)Team);
}
