#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveVector.h"
#include "Components/Movement/BaseMovementComponent.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HomeWorkTypes.h"
#include "GenericTeamAgentInterface.h"

#include "BaseCharacter.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAimingStateChanged, bool)

class UBaseMovementComponent;
class AInteractiveActor;
class UAnimMontage;
class UCharacterEquipmentComponent;
class UCharacterAttributesComponent;

typedef TArray<AInteractiveActor*, TInlineAllocator<10>> TLaddersArray;

USTRUCT(BlueprintType)
struct FMantlingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* FPSMantlingMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UCurveVector* Curve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.f, UIMin = 0.f))
	float MaxHeight = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.f, UIMin = 0.f))
	float MinHeight = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.f, UIMin = 0.f))
	float MaxHeightStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.f, UIMin = 0.f))
	float MinHeightStartTime = 0.5f;

	// offset from the target ledge in direction of normal of vertical surface we had met with
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.f, UIMin = 0.f))
	float AnimCorrectXY = 65.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.f, UIMin = 0.f))
	float AnimCorrectZ = 200.f;
};


// Abstract class can not be instantiated
UCLASS(ABSTRACT, NotBlueprintable)
class HOMEWORK1_API ABaseCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* Ctrl) override;

	virtual void MoveForward(float Value) {};
	virtual void MoveRight(float Value) {};
	virtual void Turn(float Value) {};
	virtual void LookUp(float Value) {};

	virtual void TurnAtRate(float Value) {};
	virtual void LookUpAtRate(float Value) {};

	virtual void Prone2Crouch();
	virtual void Prone();

	virtual void StartSprint();
	virtual void StopSprint();

	virtual void SwimForward(float Value) {};
	virtual void SwimRight(float Value) {};
	virtual void SwimUp(float Value) {};
	
	void StartFire();
	void StopFire();	
	void StartAiming();
	void StopAiming();
	bool FORCEINLINE IsAiming() const { return bIsAiming; }

	void Reload();

	FOnAimingStateChanged OnAimingStateChanged;

	float GetAimingMovementSpeed()  const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
	void OnStartAiming();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
	void OnStopAiming();


	virtual void Tick(float DeltaSeconds) override;

	void Mantle(bool bForce);
	bool CanMantle() const; 

	void StartSlide();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE UBaseMovementComponent* GetMoveComp() { return BaseCharacterMovementComponent; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetLeftIKOffset() const { return LeftIKOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetRightIKOffset() const { return RightIKOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE float GetPelvisIKOffset() const { return PelvisIKOffset; }
		
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE FRotator GetFootRotationDegree() const { return FootRotationDegree; }

	// to allow to jump in crouch state
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	FORCEINLINE class ULedgeDetectorComponent* GetLedgeDetectorComponent() const { return LedgeDetectorComponent; }

	FORCEINLINE const FMantlingSettings& GetMantlingSettings(float LedgeHeight) const { return (LedgeHeight > LowMantleMaxHeight) ? HighMantleCfg : LowMantleCfg; };

	// LADDERS
	TLaddersArray AvailableInteractiveActors;

	void RegisterInteractiveActor(AInteractiveActor* Actor);
	void UnRegisterInteractiveActor(AInteractiveActor* Actor);

	void ClimbLadderUp(float Value);
	void InteractLadder();
	void InteractZipline();

	const class ALadder* GetAvailableLadder() const;
	const class AZipline* GetAvailableZipline() const;

	void AttachZipline();
	void SetBaseTranslationOffsetZ(float OffsetZ) { BaseTranslationOffset.Z = OffsetZ; };
	void SetSpringArmOffsetZ(float OffsetZ);
	const ACharacter* GetDefaultCharacter() const { return GetDefault<ACharacter>(GetClass()); };

	virtual void Falling() override; // called on falling/jumping begin
	virtual void Landed(const FHitResult& Hit) override; // Hit is a ground point after falling
	virtual void NotifyJumpApex() override; // when we start falling after jump

	FORCEINLINE bool IsOutOfStamina() const { return bIsOutOfStamina; };
	void SetOutOfStamina(bool Value);
	bool IsSwimmingUnderWater() const;

	const UCharacterEquipmentComponent* GetEquipmentComp() const { return EquipmentComponent; }
	UCharacterEquipmentComponent* GetEquipmentComp_Mutable() const { return EquipmentComponent; }

	const UCharacterAttributesComponent* GetCharacterAttributesComponent() const { return CharacterAttributesComponent; }
	UCharacterAttributesComponent* GetCharacterAttributesComponent_Mutable() const { return CharacterAttributesComponent; }

	void NextItem();
	void PrevItem();
	void EquipSideArm();
	void EquipPrimaryWeapon();
	void EquipSecondaryWeapon();
	void EquipShotgunWeapon();
	void EquipPrimaryItem();
	void Unequip();
	void SwitchWeaponMode();
	
	void PrimaryMeleeAttack();
	void SecondaryMeleeAttack();

	/** IGenericTeamAgentInterface */
	virtual FGenericTeamId GetGenericTeamId() const override;
	/** ~IGenericTeamAgentInterface */

protected:

	virtual void OnStartAimingInternal();
	virtual void OnStopAimingInternal();

	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	virtual void OnStartProne(float HeightAdjust, float ScaledHeightAdjust);
	virtual void OnEndProne(float HeightAdjust, float ScaledHeightAdjust);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Team")
	ETeams Team = ETeams::Enemy;

	// BlueprintImplementableEvent can be used to hook event in the Blueprint only
	UFUNCTION(BlueprintNativeEvent, Category = "Character | Movement")
	void OnSprintStarted();
	virtual void OnSprintStarted_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Character | Movement")
	void OnSprintStopped();
	virtual void OnSprintStopped_Implementation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Controls")
	float BaseTurnRate = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Controls")
	float BaseLookUpRate = 45.f;

	virtual bool CanSprint();
	virtual bool CanCrouch() const override;

	UBaseMovementComponent* BaseCharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Movement")
	class ULedgeDetectorComponent* LedgeDetectorComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSettings HighMantleCfg;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSettings LowMantleCfg;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling", meta=(ClampMin=0.f, UIMin=0.f))
	float LowMantleMaxHeight = 125.f; // if ledge height is more, we use HighMantleCfg

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Camera", meta=(UIMin=0.0f, ClampMin=0.0f))
	float SprintArmLength = 500.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Camera")
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Camera")
	class USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Camera")
	UCurveFloat* TimelineCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Camera")
	float PlayRate = 1.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK Settings")
	FName LeftIKSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK Settings")
	FName RightIKSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | IK Settings", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float IKInterpSpeed = 10.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character | IK Settings | Debug")
	float LeftIKOffset = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character | IK Settings | Debug")
	float RightIKOffset = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character | IK Settings | Debug")
	float PelvisIKOffset = 0.f;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character | IK Settings | Debug")
	float IKTraceDistance = 50.f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Movement | Slide", meta = (ClampMin = 0.f, UIMin = 0.f))
	UAnimMontage* SlideMontage;

	// FPS Mantling
	virtual void OnMantle(const FMantlingSettings& MantlingSettings, float AnimStartTime);

	// hard landing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Movement | Hard landing", meta = (ClampMin = 0.f, UIMin = 0.f))
	float HardlandingMinHeight = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Movement | Hard landing", meta = (ClampMin = 0.f, UIMin = 0.f))
	UAnimMontage* HardLandingMontage;
	
	float topReachedZ = 0.f; // hard landing

	virtual bool IsAnyMontagePlaying() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	class UCharacterAttributesComponent* CharacterAttributesComponent;

	virtual void OnDeath(bool bIsSwimming);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Animations")
	class UAnimMontage* OnDeathAnimMontage;

	// Damage depending from fall height (in meters)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Attributes")
	class UCurveFloat* FallDamageCurve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterEquipmentComponent* EquipmentComponent;

private:
	bool bDebug = true;
	bool bIsSprintRequested = false;
	
	void TryChangeSprintState();

	float DefaultArmLength = 0.f;

	FTimeline SprintArmTimeline;

	void SprintArmUpdate(const float Alpha);

	float CountOffset(FName SocketName);

	FRotator FootRotationDegree = FRotator::ZeroRotator;

	FTimerHandle LadderTimerHandle;
	void EndTopLadderAttach();
	
	UPROPERTY()
	const ALadder* CurrentLadder = nullptr;

	//FTimerHandle DeathMontageTimer;

	void EnableRagdoll();

	FVector CurrentFallApex;

	bool bIsOutOfStamina = false;

	bool bIsAiming = false;
	float CurrentAimingMovementSpeed;
};
