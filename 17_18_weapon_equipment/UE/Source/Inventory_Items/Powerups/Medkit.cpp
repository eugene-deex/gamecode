#include "Medkit.h"
#include "Components/CharacterComponents/CharacterAttributesComponent.h"
#include "Characters/BaseCharacter.h"

bool UMedkit::Consume(ABaseCharacter* Consumer)
{
	UCharacterAttributesComponent* AttrComp = Consumer->GetCharacterAttributesComponent_Mutable();
	AttrComp->AddHealth(Health);
	this->ConditionalBeginDestroy();
	return true;
}
