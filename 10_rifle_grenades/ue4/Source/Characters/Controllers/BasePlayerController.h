#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BasePlayerController.generated.h"

UCLASS()
class HOMEWORK1_API ABasePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void SetPawn(APawn* InPawn) override;
	float GetForwardAxis() const { return ForwardAxis; };
	float GetRightAxis() const { return RightAxis; };

	FORCEINLINE bool GetIgnoreCameraPitch() const { return bIgnoreCameraPitch; }
	FORCEINLINE void SetIgnoreCameraPitch(bool Value) { bIgnoreCameraPitch = Value; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<class UPlayerHUDWidget> PlayerHUDWidgetClass;

	virtual void SetupInputComponent() override;

private:
	UPlayerHUDWidget* PlayerHUDWidget = nullptr;
	void CreateAndInitWidgets();

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void Jump();
	void CrouchProne();
	void ClearProneTime();

	void StartSprint();
	void StopSprint();
	void ProneLongPress();

	// swimming
	void SwimForward(float Value);
	void SwimRight(float Value);
	void SwimUp(float Value);

	void Mantle();

	// ladders
	void ClimbLadderUp(float Value);
	void InteractLadder();
	void InteractZipline();
	
	void Slide();

	void PlayerStartFire();
	void PlayerStopFire();

	void StartAiming();
	void StopAiming();
	void Reload();
	void NextItem();
	void PrevItem();
	void EquipSideArm();
	void EquipPrimaryWeapon();
	void EquipSecondaryWeapon();
	void EquipShotgunWeapon();
	void EquipPrimaryItem();
	void Unequip();
	void SwitchWeaponMode();

	TSoftObjectPtr<class ABaseCharacter> CharCached;

	FTimerHandle ProneTimerHandle;
	float ProneDelay = 0.5f;

	// wallrun
	float ForwardAxis = 0.f;
	float RightAxis = 0.f;

private:
	bool bIgnoreCameraPitch = false;
};
