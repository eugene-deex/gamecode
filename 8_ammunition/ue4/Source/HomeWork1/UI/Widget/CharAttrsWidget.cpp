#include "CharAttrsWidget.h"
#include "Characters/BaseCharacter.h"
#include "Components/CharacterComponents/CharacterAttributesComponent.h"

float UCharAttrsWidget::GetHealthPercent() const
{
	return Health / 100.f;
}

float UCharAttrsWidget::GetStaminaPercent() const
{
	return Stamina / 100.f;
}

float UCharAttrsWidget::GetOxygenPercent() const
{
	return Oxygen / 100.f;
}
