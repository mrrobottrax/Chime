#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CommandBase.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class CHIME_API UCommandBase : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void ExecuteCommand(AActor* Executor, const TArray<FString>& Args);
};
