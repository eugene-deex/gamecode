#include "BaseMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveVector.h"
#include "HomeWork1/Characters/BaseCharacter.h"
#include "HomeWork1/Utils/DeexUtils.h"
#include "DrawDebugHelpers.h"
#include "HomeWork1/Actors/Ladder.h"
#include "HomeWork1/Actors/Zipline.h"
#include "Kismet/GameplayStatics.h"
#include "HomeWork1/Characters/Controllers/BasePlayerController.h"
#include "HomeWork1/HomeWorkTypes.h"
#include "HomeWork1/HomeWork1.h"
#include "Net/UnrealNetwork.h"


UBaseMovementComponent::UBaseMovementComponent()
{
	SetIsReplicatedByDefault(true);
}

void UBaseMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ActorScaleZ = GetOwner()->GetActorScale3D().Z;

	// RADIUS
	UCapsuleComponent* DefaultCapsuleComponent = GetOwner()->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent();
	CapsuleRadiusScaled = DefaultCapsuleComponent->GetUnscaledCapsuleRadius() * ActorScaleZ; // default radius 34
	ProneCapsuleRadiusScaled = 40.f * ActorScaleZ;
	
	// HALF STAY
	StayedHalfHeightScaled = DefaultCapsuleComponent->GetUnscaledCapsuleHalfHeight() * ActorScaleZ; // 90

	// HALF CROUCH
	CrouchedHalfHeightScaled = CrouchedHalfHeight * ActorScaleZ; // 60
	
	// HALF PRONE
	PronedHalfHeightScaled = 40.f * ActorScaleZ; // 40

	// HALF SLIDE
	SlideHalfHeightScaled = SlideCaspsuleHalfHeight * ActorScaleZ; // 60

	CachedChar = StaticCast<ABaseCharacter*>(GetOwner());

	// wallrun timeline
	if (IsValid(WallrunTimelineCurve))
	{
		FOnTimelineFloat TimelineCallback;
		WallrunTimeline.AddInterpFloat(WallrunTimelineCurve, TimelineCallback);
	}
}

void UBaseMovementComponent::StartMantle(const FMantlingParameters& Params)
{
	//bIsMantling = true;
	CurrentMantlingParams = Params;
	SetMovementMode(EMovementMode::MOVE_Custom, (uint8) ECustomMovementMode::CMOVE_Mantling);
}

void UBaseMovementComponent::EndMantle()
{
	bIsMantling = false;
	SetMovementMode(EMovementMode::MOVE_Walking);
}

bool UBaseMovementComponent::IsMantling() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Mantling;
}

void UBaseMovementComponent::AttachToZipline(const AZipline* Zipline)
{
	CurrentZipline = Zipline;
	
	FVector ZiplineForward = CurrentZipline->GetMoveVector();
	ZiplineForward.Normalize();

	float Projection = GetActorToZiplineProjection(GetActorLocation());

	FVector NewCharLoc = CurrentZipline->GetTopColumnPos() + Projection * ZiplineForward + CurrentZipline->GetTopHangOffset();
	GetOwner()->SetActorLocation(NewCharLoc);

	FRotator TargetOrientRot = CurrentZipline->GetMoveVector().ToOrientationRotator();
	GetOwner()->SetActorRotation(TargetOrientRot);

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Zipline);
}

float UBaseMovementComponent::GetActorToZiplineProjection(FVector ActorLoc)
{
	FVector ZiplineForward = CurrentZipline->GetMoveVector();
	ZiplineForward.Normalize();

	FVector ZiplineDistance = ActorLoc - CurrentZipline->GetTopColumnPos();
	float Projection = FVector::DotProduct(ZiplineForward, ZiplineDistance); // dot product - projection of vector from zip start to player center - on Forward (pitch?)

	return Projection;
}

void UBaseMovementComponent::AttachToLadder(const ALadder* Ladder)
{
	CurrentLadder = Ladder;
	
	// rotate player to the ladder
	FRotator TargetOrientRot = CurrentLadder->GetActorForwardVector().ToOrientationRotator();
	TargetOrientRot.Yaw += 180;

	// set right player position
	float Actor2LadderProj = GetActorToLadderProjection(GetActorLocation());
	FVector Up = CurrentLadder->GetActorUpVector();
	FVector Forward = CurrentLadder->GetActorForwardVector();
	FVector NewCharLoc = CurrentLadder->GetActorLocation() + Actor2LadderProj * Up + Ladder2CharOffset * Forward;

	if (CurrentLadder->GetIsOnTop())
	{
		NewCharLoc = CurrentLadder->GetAnimMontageStartLoc();
	}

	GetOwner()->SetActorRotation(TargetOrientRot);
	GetOwner()->SetActorLocation(NewCharLoc);

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Ladder);
}

float UBaseMovementComponent::GetActorToLadderProjection(const FVector& Location) const
{
	checkf(IsValid(CurrentLadder), TEXT("UBaseMovementComponent::GetCharacterToLadderProjection() can not be invoked, CurrentLadder is nullptr"));

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector Ladder2CharDist = Location - CurrentLadder->GetActorLocation(); // from ladder bottom to player center
	return FVector::DotProduct(LadderUpVector, Ladder2CharDist); // dot product - projection of vector from ladder bottom to player center - on Yaw
}

float UBaseMovementComponent::GetLadderSpeedRatio() const
{
	checkf(IsValid(CurrentLadder), TEXT("UBaseMovementComponent::GetLadderSpeedRatio() can not be invoked, CurrentLadder is nullptr"));

	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	return FVector::DotProduct(LadderUpVector, Velocity) / LadderSpeed;
}

void UBaseMovementComponent::DetachFromZipline()
{
	ZiplineAcceleration = 0.f;
	SetMovementMode(MOVE_Falling);
}

void UBaseMovementComponent::DetachFromLadder(EDetachLadderMethod Method)
{
	if (Method == EDetachLadderMethod::Fall) {
		SetMovementMode(MOVE_Falling);

	} else if (Method == EDetachLadderMethod::ReachTop) {
		CachedChar->Mantle(true);

	} else if (Method == EDetachLadderMethod::ReachBottom) {
		SetMovementMode(MOVE_Walking);

	} else if (Method == EDetachLadderMethod::JumpOff) {
		
		FVector JumpDir = CurrentLadder->GetActorForwardVector();
		SetMovementMode(MOVE_Falling);

		// to turn player smoothly in PhysicsRotation(DeltaTime) - see override
		ForceTargetRot = JumpDir.ToOrientationRotator();
		bForceRotation = true;

		Launch(JumpDir * JumpOffLadderSpeed);
	}
}

bool UBaseMovementComponent::IsOnZipline() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Zipline;
}

bool UBaseMovementComponent::IsOnLadder() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Ladder;
}

const class ALadder* UBaseMovementComponent::GetCurrentLadder() const
{
	return CurrentLadder;
}

// There we have PhysWalking for MOVE_Walking, PhysSwimming for MOVE_Swimming, etc
// So we override PhysCustom for our Custom movement type and implement our new movement in Custom mode 
void UBaseMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CachedChar->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Mantling)
	{
		PhysMantling(DeltaTime, Iterations);
	}
	else if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Ladder)
	{
		PhysLadder(DeltaTime, Iterations);
	}
	else if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Zipline)
	{
		PhysZipline(DeltaTime, Iterations);
	}
	else if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Wallrun)
	{
		PhysWallrun(DeltaTime, Iterations);
	}
	else if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Slide)
	{
		PhysSlide(DeltaTime, Iterations);
	}
}

void UBaseMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{
	CalcVelocity(DeltaTime, 1.f, false, Deceleration);
	
	FVector Start = GetOwner()->GetActorLocation();

	// get new vector from current point on zipline to zipline end
	// so we don't need to take into account that ActorLocation is in the center of the body - we use projection on zipline
	FVector ZiplineForward = CurrentZipline->GetMoveVector();
	ZiplineForward.Normalize();
	float Projection = GetActorToZiplineProjection(GetActorLocation());
	FVector End = CurrentZipline->GetBottomColumnPos() + Projection * ZiplineForward + CurrentZipline->GetTopHangOffset();

	float Speed = ZiplineSpeed + ZiplineCurrentAcceleration;
	ZiplineCurrentAcceleration += ZiplineAcceleration;
	FVector NewLoc = FMath::VInterpConstantTo(Start, End, DeltaTime, Speed);

	if (GetActorToZiplineProjection(NewLoc) > (CurrentZipline->GetSize() - CurrentZipline->GetZiplineDetachOffset()))
	{
		DetachFromZipline();
		return;
	}

	GetOwner()->SetActorLocation(NewLoc);
	UE_LOG(LogDeex, Warning, TEXT("speed %f | proj %f | zip length %f | break point %f"), Speed, GetActorToZiplineProjection(NewLoc), CurrentZipline->GetSize(), (CurrentZipline->GetSize() - CurrentZipline->GetZiplineDetachOffset()));
}

void UBaseMovementComponent::PhysWallrun(float DeltaTime, int32 Iterations)
{
	FVector Delta = Velocity;

	float TimelineValue = WallrunTimeline.GetPlaybackPosition();
	float CurVal = WallrunTimelineCurve->GetFloatValue(TimelineValue);
	FVector DeltaUp = GetOwner()->GetActorUpVector() * CurVal * WallrunUpRate;
	Delta += DeltaUp;
	
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta * DeltaTime, GetOwner()->GetActorRotation(), true, Hit);
}

void UBaseMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{
	FVector Delta = Velocity;
	FRotator Rot = GetOwner()->GetActorForwardVector().ToOrientationRotator(); // to move only forward
	
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta * DeltaTime, Rot, true, Hit);
}

void UBaseMovementComponent::PhysLadder(float DeltaTime, int32 Iterations)
{
	// Updates Velocity and Acceleration based on the current state, applying the effects of friction and acceleration or deceleration. Does not apply gravity
	CalcVelocity(DeltaTime, 1.f, false, Deceleration); // huge deceleration to not slide on ladder
	FVector Delta = Velocity * DeltaTime;

	if (HasAnimRootMotion())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), false, Hit);
		return;
	}

	// detect if we reach end of ladder with DotProduct
	FVector NewPos = GetActorLocation() + Delta;
	float NewPosProj = GetActorToLadderProjection(NewPos);

	if (NewPosProj < MinLadderBottomOffset) // bottom end
	{
		DetachFromLadder(EDetachLadderMethod::ReachBottom);
		return;
	}
	else if(NewPosProj > (CurrentLadder->GetLadderHeight() - MaxLadderTopOffset)) // top end of the ladder
	{
		DetachFromLadder(EDetachLadderMethod::ReachTop);
		return;
	}

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);
}

void UBaseMovementComponent::PhysMantling(float DeltaTime, int32 Iterations)
{
	// get mantling movement character position at the moment by current Timer value
	float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(MantlingTimerHandle) + CurrentMantlingParams.StartTime;

	FVector CurveValue = CurrentMantlingParams.Curve->GetVectorValue(ElapsedTime);

	float PosAlpha = CurveValue.X; // value of the curve in CurveVector asset
	float XYCorrectionAlpha = CurveValue.Y;
	float ZCorrectionAlpha = CurveValue.Z;

	// difference between positions for moving actors
	FVector Diff = CurrentMantlingParams.LedgeActor->GetActorLocation() - CurrentMantlingParams.ActorInitLoc;

	// move player from his current location (initLoc) to correct init location for this animation (200f down and 65f back from ledge)
	FVector CorrectedInitLoc = FMath::Lerp(CurrentMantlingParams.InitLoc, CurrentMantlingParams.InitAnimLoc, XYCorrectionAlpha);
	CorrectedInitLoc.Z = FMath::Lerp(CurrentMantlingParams.InitLoc.Z, CurrentMantlingParams.InitAnimLoc.Z, ZCorrectionAlpha);

	// lerp from CorrectedInitLoc to TargetLoc on ledge
	FVector NewLoc = FMath::Lerp(CorrectedInitLoc, CurrentMantlingParams.TargetLoc + Diff, PosAlpha); // move player to ledge slightly
	FRotator NewRot = FMath::Lerp(CurrentMantlingParams.InitRot, CurrentMantlingParams.TargetRot, PosAlpha);

	FVector Delta = NewLoc - GetActorLocation();
	Velocity = Delta / DeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, NewRot, false, Hit);
}

void UBaseMovementComponent::OnMovementModeChanged(EMovementMode PrevMode, uint8 PrevCustomMode)
{
	Super::OnMovementModeChanged(PrevMode, PrevCustomMode);

	if (MovementMode == MOVE_Swimming)
	{
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(SwimmingCapsuleRadius, SwimmingCapsuleHalfHeight);
	}
	else if(PrevMode == MOVE_Swimming)
	{
		UCapsuleComponent* DefaultCapsuleComponent = GetOwner()->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCapsuleComponent->GetUnscaledCapsuleHalfHeight(), true); // true - do overlap events
	}

	if (PrevMode == MOVE_Custom && PrevCustomMode == (uint8)ECustomMovementMode::CMOVE_Ladder)
	{
		CurrentLadder = nullptr;
	}

	if (PrevMode == MOVE_Custom && PrevCustomMode == (uint8)ECustomMovementMode::CMOVE_Zipline)
	{
		CurrentZipline = nullptr;
	}

	if (PrevMode == MOVE_Custom && PrevCustomMode == (uint8)ECustomMovementMode::CMOVE_Wallrun)
	{
		CurrentWallrunMode = EWallrunMode::W_Off;
		WallRunDirection = FVector::ZeroVector;
		WallRunTimerHandle.Invalidate();
	}

	if (MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Mantling)
	{
		GetWorld()->GetTimerManager().SetTimer(MantlingTimerHandle, this, &UBaseMovementComponent::EndMantle, CurrentMantlingParams.Duration, false);
	}
}

bool UBaseMovementComponent::AreWallRunKeysPressed() const
{
	ABasePlayerController* PlayerController = StaticCast<ABasePlayerController*>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	
	if (PlayerController->GetForwardAxis() < 0.1f)
	{
		return false;
	}

	if(CurrentWallrunMode == EWallrunMode::W_LeftSide && PlayerController->GetRightAxis() > 0.1f)
	{
		return false;
	}

	if (CurrentWallrunMode == EWallrunMode::W_RightSide && PlayerController->GetRightAxis() < -0.1f)
	{
		return false;
	}

	return true;
}

void UBaseMovementComponent::UpdateWallRun(float DeltaSeconds)
{
	if (IsOnWall() && !AreWallRunKeysPressed())
	{
		StopWallRun();
		return;
	}

	if (!IsOnWall())
	{
		if (WallRunTimerHandle.IsValid())
		{
			StopWallRun();
		}
		
		return;
	}
	
	WallrunTimeline.TickTimeline(DeltaSeconds);

	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector TraceDir = CurrentWallrunMode == EWallrunMode::W_LeftSide ? -GetOwner()->GetActorRightVector() : GetOwner()->GetActorRightVector();
	FVector End = Start + TraceDir * WallrunTraceLength;

	if (bDebugWallrun) { DrawDebugLine(GetWorld(), Start, End, FColor::Red, true, -1, 0, 5); }

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Wallrunnable);
	if (bHit)
	{
		if (!IsSurfaceWallRunnable(HitResult.Normal))
		{
			StopWallRun();
			return;
		}

		EWallrunMode PrevMode = CurrentWallrunMode;
		GetWallSideAndDir(HitResult.Normal);
		if (CurrentWallrunMode != PrevMode)
		{
			StopWallRun();
			return;
		}

		FRotator Rot = WallRunDirection.ToOrientationRotator();
		FRotator RollTune = HitResult.Normal.ToOrientationRotator();

		if (CurrentWallrunMode == EWallrunMode::W_RightSide)
		{
			Rot.Yaw -= 180.f;
			Rot.Roll += RollTune.Pitch;
		}
		else
		{
			Rot.Roll -= RollTune.Pitch;
		}

		GetOwner()->SetActorRotation(Rot);

		if (CurrentWallrunMode == EWallrunMode::W_LeftSide)
		{
			Velocity = GetMaxSpeed() * WallRunDirection;
		}
		else {
			Velocity = GetMaxSpeed() * -WallRunDirection;
		}

	}
	else {
		StopWallRun();
	}
}

// called from ABaseCharacter::OnOverlapBegin
void UBaseMovementComponent::StartWallrunOnHit(AActor* WallActor, const FVector& HitNormal)
{

	if (LastWallActor.IsValid() && LastWallActor != WallActor)
	{
		if (bDebugWallrun) { UE_LOG(LogDeex, Warning, TEXT("FORCE UNLOCK")); }
		WallRunTimerHandle.Invalidate();
		UnLockWallRun();
	}

	LastWallActor = WallActor;

	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector EndRight = Start + GetOwner()->GetActorRightVector() * GetWallrunTraceLength();
	FVector EndLeft = Start + -GetOwner()->GetActorRightVector() * GetWallrunTraceLength();

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, EndRight, ECC_Wallrunnable) || 
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, EndLeft, ECC_Wallrunnable))
	{
		ProcessWallrun(HitNormal);
	}
}

void UBaseMovementComponent::StartSlide(UAnimMontage* SlideMontage)
{
	if (IsFalling())
	{
		return;
	}

	//if (bIsSliding)
	//{
	//	return;
	//}
	bIsSliding = true;

	SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Slide);

	float Scale = CachedChar->GetActorScale3D().Z;
	float Radius = GetCapsuleRadiusScaled();
	float StayedHalfHeight = GetStayedHalfHeight(); // 90
	float SlideHalfHeight = GetSlideHalfHeightScaled(); // 40
	CachedChar->GetCapsuleComponent()->SetCapsuleSize(Radius, SlideHalfHeight, true);

	float Adjust = StayedHalfHeight - SlideHalfHeight;
	
	CachedChar->SetSpringArmOffsetZ(-Adjust);

	// move component down
	USceneComponent* Scene = UpdatedComponent;
	Scene->MoveComponent(FVector(0.f, 0.f, -Adjust), Scene->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);

	CachedChar->RecalculateBaseEyeHeight();

	const ACharacter* DefaultChar = CachedChar->GetDefaultCharacter();
	if (CachedChar->GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = CachedChar->GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = DefaultChar->GetMesh()->GetRelativeLocation().Z + Adjust;
		CachedChar->SetBaseTranslationOffsetZ(MeshRelativeLocation.Z);
	} else	{
		CachedChar->SetBaseTranslationOffsetZ(DefaultChar->GetBaseTranslationOffset().Z + Adjust);
	}

	CachedChar->PlayAnimMontage(SlideMontage);
}

void UBaseMovementComponent::StopSlide()
{
	UE_CLOG(bDebugWallrun, LogDeex, Warning, TEXT("UBaseMovementComponent::StopSlide()"));

	bIsSliding = false;

	SetMovementMode(EMovementMode::MOVE_Walking);

	const FVector PawnLocation = GetActorLocation(); // GetCharacterMovement()->UpdatedComponent->GetComponentLocation();
	const float SweepInflation = KINDA_SMALL_NUMBER * 10.f;
	const float MinFloorDistanceHalf = (UCharacterMovementComponent::MIN_FLOOR_DIST / 2.f); // 1.9f/2.0

	const float FloorZ = PawnLocation.Z - GetSlideHalfHeightScaled() + GetCrouchedHalfHeightScaled() + SweepInflation + MinFloorDistanceHalf;
	const FVector Start = FVector(PawnLocation.X, PawnLocation.Y, FloorZ);
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	FHitResult OutHit;

	float Scale = CachedChar->GetActorScale3D().Z;
	float StayedHalfHeight = GetStayedHalfHeight(); // 90
	float SlideHalfHeight = GetSlideHalfHeightScaled(); // 30

	float Adjust = StayedHalfHeight - SlideHalfHeight; // 90-60=30
	float NewCapsuleSize = StayedHalfHeight;

	CachedChar->SetSpringArmOffsetZ(Adjust);

	// move component up
	USceneComponent* Scene = UpdatedComponent;
	Scene->MoveComponent(FVector(0.f, 0.f, Adjust), Scene->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);

	//if (UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Start, Start, GetCapsuleRadiusScaled(), GetStayedHalfHeight(), TraceType, false, TArray<AActor*>(), EDrawDebugTrace::ForDuration, OutHit, true, FLinearColor::Red, FLinearColor::Blue, 5.f))
	//{
	//	UE_CLOG(bDebug, LogDeex, Warning, TEXT("Can't stand up here, crouch"));
	//	NewCapsuleSize = CrouchedHalfHeight;
	//	Crouch();
	//}

	// Adjust
	CachedChar->RecalculateBaseEyeHeight();

	const ACharacter* DefaultChar = CachedChar->GetDefaultCharacter();
	if (CachedChar->GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = CachedChar->GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = DefaultChar->GetMesh()->GetRelativeLocation().Z; //  - Adjust
		CachedChar->SetBaseTranslationOffsetZ(MeshRelativeLocation.Z);
	}
	else
	{
		CachedChar->SetBaseTranslationOffsetZ(DefaultChar->GetBaseTranslationOffset().Z); //  - Adjust
	}

	CachedChar->GetCapsuleComponent()->SetCapsuleSize(GetCapsuleRadiusScaled(), NewCapsuleSize);

	UAnimInstance* AnimInst = CachedChar->GetMesh()->GetAnimInstance();
	AnimInst->StopAllMontages(0.5f);
}

void UBaseMovementComponent::OnRep_IsMantling(bool bWasMantling)
{
	if (CachedChar->GetLocalRole() == ROLE_SimulatedProxy && !bWasMantling && bIsMantling)
	{
		CachedChar->Mantle(true);
	}
}

void UBaseMovementComponent::OnRep_IsSliding(bool bWasSliding)
{
	if (CachedChar->GetLocalRole() == ROLE_SimulatedProxy && !bWasSliding && bIsSliding)
	{
		CachedChar->StartSlide();
	}
}

void UBaseMovementComponent::StartWallRun()
{
	if (bIsWallrunBlocked)
	{
		UE_CLOG(bDebugWallrun, LogDeex, Warning, TEXT("UBaseMovementComponent::StartWallRun() wallrun locked"));
		return;
	}

	FRotator Rot = WallRunDirection.ToOrientationRotator();

	if (CurrentWallrunMode == EWallrunMode::W_RightSide)
	{
		Rot.Yaw -= 180.f;
		Rot.Roll += WallrunPitchTune;
	}
	else
	{
		Rot.Roll -= WallrunPitchTune;
	}

	GetOwner()->SetActorRotation(Rot);
	SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Wallrun);

	GetWorld()->GetTimerManager().SetTimer(WallRunTimerHandle, this, &UBaseMovementComponent::StopWallRun, MaxWallRunTime, false);	
	WallrunTimeline.PlayFromStart();
	bIsWallrun = true;
}

void UBaseMovementComponent::StopWallRun()
{
	StopWallRun(false);
}

void UBaseMovementComponent::StopWallRun(bool bIsJumpOut)
{
	if (bDebugWallrun) { UE_LOG(LogDeex, Warning, TEXT("STOP WALLRUN")); }
	WallRunTimerHandle.Invalidate();

	if (bIsJumpOut)
	{
		FVector JumpDir = CurrentWallrunMode == EWallrunMode::W_LeftSide ? GetOwner()->GetActorRightVector() : -GetOwner()->GetActorRightVector();
		JumpDir += FVector::UpVector;
		ForceTargetRot = JumpDir.ToOrientationRotator();
		bForceRotation = true;
		Launch(JumpDir * WallrunJumpOffSpeed);
	}
	else 
	{	
		if (WallrunLockTime > 0.01f)
		{
			LockWallRun();
			GetWorld()->GetTimerManager().SetTimer(WallRunTimerLockHandle, this, &UBaseMovementComponent::UnLockWallRun, WallrunLockTime, false);
		}
	}

	if (MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Wallrun)
	{
		SetMovementMode(EMovementMode::MOVE_Falling);
	}

	CurrentWallrunMode = EWallrunMode::W_Off;
	WallRunDirection = FVector::ZeroVector;
	bIsWallrun = false;
}

void UBaseMovementComponent::ProcessWallrun(const FVector& HitNormal)
{
	if (IsOnWall())
	{
		return;
	}

	if (!IsSurfaceWallRunnable(HitNormal))
	{
		if (CurrentWallrunMode != EWallrunMode::W_Off)
		{
			StopWallRun();
		}
		return;
	}

	if (!IsFalling())
	{
		return;

	}

	GetWallSideAndDir(HitNormal);

	if (!AreWallRunKeysPressed())
	{
		return;
	}

	WallrunPitchTune = HitNormal.ToOrientationRotator().Pitch;
	StartWallRun();
}

void UBaseMovementComponent::GetWallSideAndDir(const FVector& HitNormal)
{
	float Dot = FVector::DotProduct(HitNormal, GetOwner()->GetActorRightVector());
	WallRunDirection = FVector::CrossProduct(HitNormal, FVector::UpVector).GetSafeNormal(); // get vector ortoghonal to both of these

	if (Dot < 0) // angle between Surface Normal and Player Right vector > 90, so surface at right side
	{
		CurrentWallrunMode = EWallrunMode::W_RightSide;
	}
	else
	{ // angle < 90, so surface at left side
		CurrentWallrunMode = EWallrunMode::W_LeftSide;
	}
}

void UBaseMovementComponent::LockWallRun()
{
	if (bDebugWallrun) { UE_LOG(LogDeex, Warning, TEXT("====== LOCK wallrun ===")); }
	bIsWallrunBlocked = true;
}

void UBaseMovementComponent::UnLockWallRun()
{
	//if (bDebugWallrun) { UE_LOG(LogDeex, Warning, TEXT("====== UN LOCK wallrun ===")); }
	bIsWallrunBlocked = false;
	WallRunTimerLockHandle.Invalidate();
}

bool UBaseMovementComponent::IsOnWall() const
{
	return UpdatedComponent&& MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Wallrun;
}

bool UBaseMovementComponent::IsSurfaceWallRunnable(const FVector& SurfaceNormal) const
{
	return SurfaceNormal.Z < GetWalkableFloorZ() && SurfaceNormal.Z > -0.005f;
}

FNetworkPredictionData_Client* UBaseMovementComponent::GetPredictionData_Client() const 
{
	if (ClientPredictionData == nullptr)
	{
		UBaseMovementComponent* MutableThis = const_cast<UBaseMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character_Homework(*this);
	}

	return ClientPredictionData;
}

void UBaseMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateBasedMovement(Flags);

	/*
	FLAG_Reserved_1		= 0x04,	// Reserved for future use
	FLAG_Reserved_2		= 0x08,	// Reserved for future use
	// Remaining bit masks are available for custom flags.
	FLAG_Custom_0		= 0x10, - Sprinting
	FLAG_Custom_1		= 0x20, - Mantling
	FLAG_Custom_2		= 0x40, - Sliding
	FLAG_Custom_3		= 0x80,
	*/

	bool bWasMantling = bIsMantling;
	bool bWasSliding = bIsSliding;
	bIsSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bIsMantling = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bIsSliding = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;

	if (CachedChar->GetLocalRole() == ROLE_Authority) // execute on the server only
	{
		if (!bWasMantling && bIsMantling)
			CachedChar->Mantle(true);

		if (!bWasSliding && bIsSliding)
			CachedChar->StartSlide();
	}
}

void UBaseMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (bForceRotation)
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation(); // Normalized
		CurrentRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): CurrentRotation"));

		FRotator DeltaRot = GetDeltaRotation(DeltaTime);
		DeltaRot.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): GetDeltaRotation"));
	
		// Accumulate a desired new rotation.
		const float AngleTolerance = 1e-3f;

		if (!CurrentRotation.Equals(ForceTargetRot, AngleTolerance))
		{
			FRotator DesiredRotation = ForceTargetRot;

			// PITCH
			if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, DesiredRotation.Pitch, AngleTolerance))
			{
				DesiredRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
			}

			// YAW
			if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw, AngleTolerance))
			{
				DesiredRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
			}

			// ROLL
			if (!FMath::IsNearlyEqual(CurrentRotation.Roll, DesiredRotation.Roll, AngleTolerance))
			{
				DesiredRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
			}

			// Set the new rotation.
			DesiredRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): DesiredRotation"));
			MoveUpdatedComponent(FVector::ZeroVector, DesiredRotation, /*bSweep*/ false);
		}
		else 
		{
			ForceTargetRot = FRotator::ZeroRotator;
			bForceRotation = false;
		}
		return;
	}

	if (IsOnLadder() || IsOnZipline())
	{
		return;
	}

	Super::PhysicsRotation(DeltaTime);
}

void UBaseMovementComponent::SetCurrentPlayerState(EPlayerState State)
{
	CurrentState = State;
}

void UBaseMovementComponent::StartSprint()
{
	if (MovementMode != MOVE_Swimming && MovementMode != MOVE_Walking)
	{
		return;
	}

	bIsSprinting = true;
	bForceMaxAccel = 1;
}

void UBaseMovementComponent::StopSprint()
{
	bIsSprinting = false;
	bForceMaxAccel = 0;
}

float UBaseMovementComponent::GetMaxSpeed() const
{
	if (bIsSliding)
	{
		return SlideSpeed;
	}

	if (CachedChar.IsValid())
	{
		if (CachedChar->IsOutOfStamina())
		{
			return MaxSpeedOutOfStamina;
		}

		if (CachedChar->IsAiming())
		{
			return CachedChar->GetAimingMovementSpeed();
		}
	}

	if (IsOnWall())
	{
		return WallrunSpeed;
	}

	if (CurrentState == EPlayerState::Prone)
	{
		return CrawlSpeed;
	}

	if (bIsSprinting)
	{
		return SprintSpeed;
	}

	if (IsOnLadder())
	{
		return LadderSpeed;
	}

	return Super::GetMaxSpeed();
}

void FSavedMove_Homework::Clear()
{
	Super::Clear();
	bSavedIsSprinting = 0;
	bSavedIsMantling = 0;
	bSavedIsSliding = 0;
}

uint8 FSavedMove_Homework::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	/*
		FLAG_Reserved_1		= 0x04,	// Reserved for future use
		FLAG_Reserved_2		= 0x08,	// Reserved for future use
		// Remaining bit masks are available for custom flags.
		FLAG_Custom_0		= 0x10, - Sprinting
		FLAG_Custom_1		= 0x20, - Mantling
		FLAG_Custom_2		= 0x40, - Sliding
		FLAG_Custom_3		= 0x80,
	*/

	if (bSavedIsSprinting)
	{
		Result |= FLAG_Custom_0;
	}

	if (bSavedIsMantling)
	{
		Result &= ~FLAG_JumpPressed;
		Result |= FLAG_Custom_1;
	}

	if (bSavedIsSliding)
	{
		Result |= FLAG_Custom_2;
	}

	return Result;
}

bool FSavedMove_Homework::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Homework* NewMove = StaticCast<const FSavedMove_Homework*>(NewMovePtr.Get());

	if (bSavedIsSprinting != NewMove->bSavedIsSprinting || 
		bSavedIsMantling != NewMove->bSavedIsMantling ||
		bSavedIsSliding != NewMove->bSavedIsSliding 
	)
		return false;

	return Super::CanCombineWith(NewMovePtr, InCharacter, MaxDelta);
}

void FSavedMove_Homework::SetMoveFor(ACharacter* InChar, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(InChar, InDeltaTime, NewAccel, ClientData);

	check(InChar->IsA<ABaseCharacter>());

	UBaseMovementComponent* MoveComp = StaticCast<UBaseMovementComponent*>(InChar->GetMovementComponent());
	bSavedIsSprinting = MoveComp->bIsSprinting;
	bSavedIsMantling = MoveComp->bIsMantling; // LECCION: static cast inChar to ABaseCharacter and get bIsMantling from it	
	bSavedIsSliding = MoveComp->bIsSliding;
}

void FSavedMove_Homework::PrepMoveFor(ACharacter* Char)
{
	Super::PrepMoveFor(Char);
	UBaseMovementComponent* MoveComp = StaticCast<UBaseMovementComponent*>(Char->GetMovementComponent());
	MoveComp->bIsSprinting = bSavedIsSprinting;
	MoveComp->bIsMantling = bSavedIsMantling; // TEST: was skipped in leccion
	MoveComp->bIsSliding = bSavedIsSliding;

	UE_LOG(LogDeex, Warning, TEXT("PrepMoveFor playerId: %d; bIsSliding set to %s"),
		DeexUtils::GetPlayerId(Char),
		DeexUtils::LOG(MoveComp->bIsSliding)
	);
}

FNetworkPredictionData_Client_Character_Homework::FNetworkPredictionData_Client_Character_Homework(const UCharacterMovementComponent& ClientMovement) 
	: Super(ClientMovement)
{}

FSavedMovePtr FNetworkPredictionData_Client_Character_Homework::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Homework());
}

void UBaseMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBaseMovementComponent, bIsMantling);
	DOREPLIFETIME(UBaseMovementComponent, bIsSliding);
}
