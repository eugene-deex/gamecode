#pragma once
#include "Blueprint/DragDropOperation.h"
#include "ItemDragOp.generated.h"

UCLASS()
class UItemDragOp : public UDragDropOperation
{
	GENERATED_BODY()
public:
	UPROPERTY()
	int32 Count;
};