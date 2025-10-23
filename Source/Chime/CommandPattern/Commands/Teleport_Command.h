#pragma once

#include "CoreMinimal.h"
#include "Chime/CommandPattern/CommandBase.h"
#include "Teleport_Command.generated.h"

UCLASS()
class CHIME_API UTeleport_Command : public UCommandBase
{
	GENERATED_BODY()

public:
	virtual void ExecuteCommand_Implementation(AActor* Executor, const TArray<FString>& Args) override;
};