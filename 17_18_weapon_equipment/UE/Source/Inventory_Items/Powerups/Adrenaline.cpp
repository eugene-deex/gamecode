#include "Adrenaline.h"
#include "Components/CharacterComponents/CharacterAttributesComponent.h"
#include "Characters/BaseCharacter.h"

bool UAdrenaline::Consume(ABaseCharacter* Consumer)
{
	UCharacterAttributesComponent* AttrComp = Consumer->GetCharacterAttributesComponent_Mutable();
	AttrComp->RestoreFullStamina();
	this->ConditionalBeginDestroy();
	return true;
}
