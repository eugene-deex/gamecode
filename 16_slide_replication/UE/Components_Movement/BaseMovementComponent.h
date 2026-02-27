#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LedgeDetectorComponent.h"
#include "Components/TimelineComponent.h"
#include "Interfaces/NetworkPredictionInterface.h"
#include "BaseMovementComponent.generated.h"


struct FMantlingParameters
{
	FVector ActorInitLoc = FVector::ZeroVector;
	FVector InitLoc = FVector::ZeroVector;
	FRotator InitRot = FRotator::ZeroRotator;

	FVector TargetLoc = FVector::ZeroVector;
	FRotator TargetRot = FRotator::ZeroRotator;

	float Duration = 1.f;
	float StartTime = 0.f;

	UCurveVector* Curve;

	FVector InitAnimLoc = FVector::ZeroVector; // location of Char to play full mantling animation (200f down and 65f back, see FMantlingSettings.AnimCorrectXY and FMantlingSettings.AnimCorrectZ
	TWeakObjectPtr<class AActor> LedgeActor;
};

UENUM(BlueprintType)
enum class EPlayerState : uint8
{
	Stay,
	Crouch,
	Prone
};

UENUM(BlueprintType)
enum class EDetachLadderMethod : uint8
{
	Fall = 0,
	ReachTop,
	ReachBottom,
	JumpOff
};

UENUM()
enum class ECustomMovementMode : uint8
{
	CMOVE_None = 0 UMETA(DisplayName = "None"),
	CMOVE_Mantling UMETA(DisplayName = "Mantling"),
	CMOVE_Ladder UMETA(DisplayName = "Ladder"),
	CMOVE_Zipline UMETA(DisplayName = "Zipline"),
	CMOVE_Wallrun UMETA(DisplayName = "Wallrun"),
	CMOVE_Slide UMETA(DisplayName = "Slide"),
	CMOVE_Max  UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EWallrunMode : uint8
{
	W_Off = 0 UMETA(DisplayName = "Off"),
	W_LeftSide UMETA(DisplayName = "Left side"),
	W_RightSide UMETA(DisplayName = "Right side"),
	W_MAX UMETA(Hidden),
};

UCLASS()
class HOMEWORK1_API UBaseMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

		friend class FSavedMove_Homework;

public:
	UBaseMovementComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags);

	virtual void PhysicsRotation(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsSliding() { return bIsSliding; }
	FORCEINLINE bool IsSprinting() { return bIsSprinting; }
	FORCEINLINE float GetSlideHalfHeightScaled() { return SlideHalfHeightScaled; }
	FORCEINLINE float GetCrouchedHalfHeightScaled() { return CrouchedHalfHeightScaled; }
	FORCEINLINE float GetStayedHalfHeight() { return StayedHalfHeightScaled; }
	FORCEINLINE float GetCapsuleRadiusScaled() { return CapsuleRadiusScaled; }
	
	FORCEINLINE float GetProneCapsuleRadiusScaled() { return ProneCapsuleRadiusScaled; }
	FORCEINLINE float GetPronedHalfHeightScaled() { return PronedHalfHeightScaled; }
	
	FORCEINLINE EPlayerState GetCurrentState() { return CurrentState; }
	void SetCurrentPlayerState(EPlayerState State);

	void StartSprint();
	void StopSprint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	float MaxSpeedOutOfStamina = 300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	float CrawlSpeed = 100.f;

	virtual float GetMaxSpeed() const override;

	virtual void BeginPlay() override;

	void StartMantle(const FMantlingParameters& Params);
	void EndMantle();
	bool IsMantling() const;

	// ladders
	void AttachToLadder(const class ALadder* Ladder);
	float GetActorToLadderProjection(const FVector& Location) const;
	float GetLadderSpeedRatio() const;
	void DetachFromLadder(EDetachLadderMethod Method = EDetachLadderMethod::Fall);
	bool IsOnLadder() const;
	const class ALadder* GetCurrentLadder() const;

	// zipline
	bool IsOnZipline() const;
	void AttachToZipline(const class AZipline* Zipline);
	void DetachFromZipline();
	float GetActorToZiplineProjection(FVector ActorLoc);

	// wallrun
	void StartWallRun();
	void StopWallRun();
	void StopWallRun(bool bIsJumpOut);
	void ProcessWallrun(const FVector& HitNormal);
	bool IsOnWall() const;
	bool IsSurfaceWallRunnable(const FVector& SurfaceNormal) const;
	EWallrunMode GetWallRunMode() const { return CurrentWallrunMode; }

	bool AreWallRunKeysPressed() const;
	float GetWallrunTraceLength() const { return WallrunTraceLength; }
	void UpdateWallRun(float DeltaSeconds);

	void StartWallrunOnHit(AActor* WallActor, const FVector& HitNormal);

	TWeakObjectPtr<AActor> LastWallActor;

	void StartSlide(UAnimMontage* SlideMontage);
	void StopSlide();

	UPROPERTY(ReplicatedUsing=OnRep_IsMantling)
	bool bIsMantling;

	UFUNCTION()
	void OnRep_IsMantling(bool bWasMantling);

	UPROPERTY(ReplicatedUsing=OnRep_IsSliding)
	bool bIsSliding;

	UFUNCTION()
	void OnRep_IsSliding(bool bWasSliding);

protected:

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void PhysMantling(float DeltaTime, int32 Iterations);
	void PhysLadder(float DeltaTime, int32 Iterations);
	void PhysZipline(float DeltaTime, int32 Iterations);
	void PhysWallrun(float DeltaTime, int32 Iterations);
	void PhysSlide(float DeltaTime, int32 Iterations);
	void GetWallSideAndDir(const FVector& HitNormal);
	void LockWallRun();
	void UnLockWallRun();

	virtual void OnMovementModeChanged(EMovementMode PrevMode, uint8 PrevCustomMode) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint", meta = (ClampMin = 0.f, UIMin = 0.f))
	float SprintSpeed = 1200.f;

	UPROPERTY(Category = "Character Movement: Swimming", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SwimmingCapsuleRadius = 60.f;

	UPROPERTY(Category = "Character Movement: Swimming", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SwimmingCapsuleHalfHeight = 60.f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float LadderSpeed = 200.f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float Deceleration = 2048.f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float Ladder2CharOffset = 60.f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxLadderTopOffset = 90.f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MinLadderBottomOffset = 90.f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float JumpOffLadderSpeed = 500.f;

	UPROPERTY(Category = "Character Movement: Zipline", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ZiplineSpeed = 100.f;

	UPROPERTY(Category = "Character Movement: Zipline", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ZiplineAcceleration = 100.f;
	
	UPROPERTY(Category = "Character Movement: WallRun", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxWallRunTime = 3.f;

	UPROPERTY(Category = "Character Movement: WallRun", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float WallrunTraceLength = 200.f;
	
	UPROPERTY(Category = "Character Movement: WallRun", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float WallrunSpeed = 500.f;

	UPROPERTY(Category = "Character Movement: WallRun", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float WallrunJumpOffSpeed = 400.f;
	
	UPROPERTY(Category = "Character Movement: WallRun", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float WallrunLockTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement: WallRun")
	UCurveFloat* WallrunTimelineCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement: WallRun")
	float WallrunUpRate = 50.f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideSpeed = 1000.0f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideCaspsuleHalfHeight = 60.0f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideMaxTime = 2.0f;

	EPlayerState CurrentState = EPlayerState::Stay;
	EWallrunMode CurrentWallrunMode = EWallrunMode::W_Off;
	FVector WallRunDirection = FVector::ZeroVector;

private:

	FTimeline WallrunTimeline;
	bool bDebug = true;
	bool bDebugWallrun = false;

	bool bIsWallrun = false;
	bool bIsSprinting = false;

	float ActorScaleZ;
	float CapsuleRadiusScaled;
	float ProneCapsuleRadiusScaled;
	float StayedHalfHeightScaled;
	float CrouchedHalfHeightScaled;
	float PronedHalfHeightScaled;
	float SlideHalfHeightScaled;

	FMantlingParameters CurrentMantlingParams;
	FTimerHandle MantlingTimerHandle;
	FTimerHandle WallRunTimerHandle;
	FTimerHandle WallRunTimerLockHandle;

	TWeakObjectPtr<class ABaseCharacter> CachedChar;

	UPROPERTY()
	const ALadder* CurrentLadder = nullptr;
	
	UPROPERTY()
	const AZipline* CurrentZipline = nullptr;

	FRotator ForceTargetRot = FRotator::ZeroRotator;
	bool bForceRotation = false;
	bool bIsWallrunBlocked = false;

	float WallrunPitchTune = 0.f;

	float ZiplineCurrentAcceleration = 0.f;
};

class FSavedMove_Homework : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;

public:
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* InChar, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Char) override;

private:
	uint8 bSavedIsSprinting : 1;
	uint8 bSavedIsMantling : 1;
	uint8 bSavedIsSliding : 1;
};

class FNetworkPredictionData_Client_Character_Homework : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:
	FNetworkPredictionData_Client_Character_Homework(const UCharacterMovementComponent& ClientMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};





