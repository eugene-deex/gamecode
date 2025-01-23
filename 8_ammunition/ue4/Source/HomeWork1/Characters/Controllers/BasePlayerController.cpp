#include "BasePlayerController.h"
#include "HomeWork1/Characters/BaseCharacter.h"
#include "HomeWork1/Components/Movement/BaseMovementComponent.h"
#include "UI/Widget/ReticleWidget.h"
#include "UI/Widget/PlayerHUDWidget.h"
#include "UI/Widget/AmmoWidget.h"
#include "UI/Widget/CharAttrsWidget.h"
#include "Components/CharacterComponents/CharacterAttributesComponent.h"

void ABasePlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	CharCached = Cast<ABaseCharacter>(InPawn);
	CreateAndInitWidgets();
}

void ABasePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent(); // creates InputComponent
	
	InputComponent->BindAxis("MoveForward", this, &ABasePlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ABasePlayerController::MoveRight);
	InputComponent->BindAxis("Turn", this, &ABasePlayerController::Turn);
	InputComponent->BindAxis("LookUp", this, &ABasePlayerController::LookUp);
	InputComponent->BindAxis("TurnAtRate", this, &ABasePlayerController::TurnAtRate);
	InputComponent->BindAxis("LookUpAtRate", this, &ABasePlayerController::LookUpAtRate);
	
	InputComponent->BindAxis("SwimForward", this, &ABasePlayerController::SwimForward);
	InputComponent->BindAxis("SwimRight", this, &ABasePlayerController::SwimRight);
	InputComponent->BindAxis("SwimUp", this, &ABasePlayerController::SwimUp);
	InputComponent->BindAxis("ClimbLadderUp", this, &ABasePlayerController::ClimbLadderUp);
	
	InputComponent->BindAction("InteractZipline", IE_Pressed, this, &ABasePlayerController::InteractZipline);
	InputComponent->BindAction("InteractLadder", IE_Pressed, this, &ABasePlayerController::InteractLadder);
	InputComponent->BindAction("Mantle", IE_Released, this, &ABasePlayerController::Mantle);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ABasePlayerController::Jump);

	InputComponent->BindAction("CrouchProne", IE_Pressed, this, &ABasePlayerController::CrouchProne);
	InputComponent->BindAction("CrouchProne", IE_Released, this, &ABasePlayerController::ClearProneTime);
	InputComponent->BindAction("Sprint", IE_Pressed, this, &ABasePlayerController::StartSprint);
	InputComponent->BindAction("Sprint", IE_Released, this, &ABasePlayerController::StopSprint);
	
	InputComponent->BindAction("Slide", IE_Pressed, this, &ABasePlayerController::Slide);
	InputComponent->BindAction("Fire", IE_Pressed, this, &ABasePlayerController::PlayerStartFire);
	InputComponent->BindAction("Fire", IE_Released, this, &ABasePlayerController::PlayerStopFire);
	InputComponent->BindAction("Aim", IE_Pressed, this, &ABasePlayerController::StartAiming);
	InputComponent->BindAction("Aim", IE_Released, this, &ABasePlayerController::StopAiming);
	
	InputComponent->BindAction("Reload", IE_Pressed, this, &ABasePlayerController::Reload);

	InputComponent->BindAction("NextItem", IE_Pressed, this, &ABasePlayerController::NextItem);
	InputComponent->BindAction("PrevItem", IE_Pressed, this, &ABasePlayerController::PrevItem);
}

void ABasePlayerController::CreateAndInitWidgets()
{
	if (!IsValid(PlayerHUDWidget))
	{
		PlayerHUDWidget = CreateWidget<UPlayerHUDWidget>(GetWorld(), PlayerHUDWidgetClass);
		if (IsValid(PlayerHUDWidget))
		{
			PlayerHUDWidget->AddToViewport();
		}
	}

	if (CharCached.IsValid() && IsValid(PlayerHUDWidget))
	{
		UReticleWidget* ReticleWidget = PlayerHUDWidget->GetReticleWidget();
		if (IsValid(ReticleWidget))
		{
			CharCached->OnAimingStateChanged.AddUFunction(ReticleWidget, FName("OnAimingStateChanged"));
		}

		UAmmoWidget* AmmoWidget = PlayerHUDWidget->GetAmmoWidget();
		if (IsValid(AmmoWidget))
		{
			UCharacterEquipmentComponent* CharEquip = CharCached->GetEquipmentComp_Mutable();
			CharEquip->OnCurrentWeaponAmmoChangedEvent.AddUFunction(AmmoWidget, FName("UpdateAmmoCounter"));
		}
		 
		UCharAttrsWidget* AttrsWidget = PlayerHUDWidget->GetAttributesWidget();
		if (IsValid(AttrsWidget))
		{
			UCharacterAttributesComponent* AttrComp = CharCached->GetCharacterAttributesComponent_Mutable();
			AttrComp->OnHealthChangedEvent.AddUFunction(AttrsWidget, FName("UpdateHealth"));
			AttrComp->OnStaminaChangedEvent.AddUFunction(AttrsWidget, FName("UpdateStamina"));
			AttrComp->OnOxygenChangedEvent.AddUFunction(AttrsWidget, FName("UpdateOxygen"));
		}
	}
}

void ABasePlayerController::MoveForward(float Value)
{
	ForwardAxis = Value;
	if (CharCached.IsValid())
	{
		CharCached->MoveForward(Value);

		if (FMath::IsNearlyZero(Value))
		{
			ABasePlayerController::StopSprint();
		}
	}
}

void ABasePlayerController::MoveRight(float Value)
{
	RightAxis = Value;
	if (CharCached.IsValid())
	{
		CharCached->MoveRight(Value);
	}
}

void ABasePlayerController::Turn(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->Turn(Value);
	}
}

void ABasePlayerController::LookUp(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->LookUp(Value);
	}
}

void ABasePlayerController::Mantle()
{
	if (CharCached.IsValid())
	{
		CharCached->Mantle(false);
	}
}

void ABasePlayerController::ClimbLadderUp(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->ClimbLadderUp(Value);
	}
}

void ABasePlayerController::InteractLadder()
{
	if (CharCached.IsValid())
	{
		CharCached->InteractLadder();
	}
}

void ABasePlayerController::InteractZipline()
{
	if (CharCached.IsValid())
	{
		CharCached->InteractZipline();
	}
}

void ABasePlayerController::Slide()
{
	if (CharCached.IsValid())
	{
		CharCached->StartSlide();
	}
}

void ABasePlayerController::PlayerStartFire()
{
	if (CharCached.IsValid())
	{
		CharCached->StartFire();
	}
}
void ABasePlayerController::PlayerStopFire()
{
	if (CharCached.IsValid())
	{
		CharCached->StopFire();
	}
}

void ABasePlayerController::StartAiming()
{
	if (CharCached.IsValid())
	{
		CharCached->StartAiming();
	}
}

void ABasePlayerController::StopAiming()
{
	if (CharCached.IsValid())
	{
		CharCached->StopAiming();
	}
}

void ABasePlayerController::Reload()
{
	if (CharCached.IsValid())
	{
		CharCached->Reload();
	}
}

void ABasePlayerController::NextItem()
{
	if (CharCached.IsValid())
	{
		CharCached->NextItem();
	}
}

void ABasePlayerController::PrevItem()
{
	if (CharCached.IsValid())
	{
		CharCached->PrevItem();
	}
}

void ABasePlayerController::Jump()
{
	if (CharCached.IsValid())
	{
		if (CharCached->GetMoveComp()->GetCurrentState() == EPlayerState::Prone)
		{
			CharCached->Prone2Crouch();
			CharCached->UnCrouch();
		}
		else if (CharCached->GetMoveComp()->IsOnWall())
		{
			CharCached->GetMoveComp()->StopWallRun(true);
		}
		else
		{
			CharCached->Jump();
		}
	}
}

void ABasePlayerController::TurnAtRate(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->TurnAtRate(Value);
	}
}

void ABasePlayerController::LookUpAtRate(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->LookUpAtRate(Value);
	}
}

void ABasePlayerController::CrouchProne()
{
	GetWorld()->GetTimerManager().SetTimer(ProneTimerHandle, this, &ABasePlayerController::ProneLongPress, ProneDelay, false);

	if (CharCached.IsValid())
	{
		EPlayerState State = CharCached->GetMoveComp()->GetCurrentState();
		
		if (State == EPlayerState::Stay)
		{
			CharCached->Crouch();
		}
		else if (State == EPlayerState::Prone)
		{
			CharCached->Prone2Crouch();
		}
		else if (State == EPlayerState::Crouch)
		{
			CharCached->UnCrouch();
		}
	}
}

void ABasePlayerController::ProneLongPress()
{
	EPlayerState State = CharCached->GetMoveComp()->GetCurrentState();

	if (State == EPlayerState::Crouch)
	{
		CharCached->Prone();
	}
}

void ABasePlayerController::ClearProneTime()
{
	GetWorld()->GetTimerManager().ClearTimer(ProneTimerHandle);
}

void ABasePlayerController::StartSprint()
{
	if (CharCached.IsValid())
	{
		CharCached->StartSprint();
	}
}

void ABasePlayerController::StopSprint()
{
	if (CharCached.IsValid())
	{
		CharCached->StopSprint();
	}
}

void ABasePlayerController::SwimForward(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->SwimForward(Value);
	}
}

void ABasePlayerController::SwimRight(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->SwimRight(Value);
	}
}

void ABasePlayerController::SwimUp(float Value)
{
	if (CharCached.IsValid())
	{
		CharCached->SwimUp(Value);
	}
}
