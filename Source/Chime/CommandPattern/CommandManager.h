#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/Object.h"
#include "CommandManager.generated.h"

class UCommandBase;

UCLASS(Blueprintable)
class CHIME_API UCommandManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection)override;

	virtual void Deinitialize() override;

	// Registers a command type under a keyword
	void RegisterCommand(FName CommandName, TSubclassOf<UCommandBase> CommandClass);

	// Parses text and executes the matching command
	UFUNCTION(BlueprintCallable, Category = "Commands")
	void ExecuteCommandString(const FString& CommandString);

protected:
	AActor* FindActorByName(UWorld* World, const FString& NameToFind);

private:
	UPROPERTY()
	TMap<FName, TSubclassOf<UCommandBase>> RegisteredCommands;
};
