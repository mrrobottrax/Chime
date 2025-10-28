#include "CommandManager.h"
#include "CommandBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Chime/CommandPattern/Commands/Teleport_Command.h"

void UCommandManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// register teleport command
	RegisterCommand("/tp", UTeleport_Command::StaticClass());
	UE_LOG(LogTemp, Log, TEXT("CommandManager initialized and teleport command registered."));

	// To Do:
	// Restart Level
	// Beat level
	// Spawn Dynamic Object
}

void UCommandManager::Deinitialize()
{
	RegisteredCommands.Empty();
	Super::Deinitialize();
}

void UCommandManager::RegisterCommand(FName CommandName, TSubclassOf<UCommandBase> CommandClass)
{
	if (!*CommandName.ToString() || !CommandClass)
		return;

	RegisteredCommands.Add(CommandName, CommandClass);
}

AActor* UCommandManager::FindActorByName(UWorld* World, const FString& NameToFind)
{
	if (!World) return nullptr;

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

	for (AActor* Actor : AllActors)
	{
		if (!Actor) continue;

		if (Actor->GetName().Contains(NameToFind))
			return Actor;
	}

	return nullptr;
}

void UCommandManager::ExecuteCommandString(const FString& CommandString)
{
	if (CommandString.IsEmpty()) return;

	TArray<FString> Tokens;
	CommandString.ParseIntoArray(Tokens, TEXT(" "), true);

	if (Tokens.Num() == 0) return;

	FName CommandName(*Tokens[0]);
	Tokens.RemoveAt(0); // Remove the command name

	if (TSubclassOf<UCommandBase>* CommandClassPtr = RegisteredCommands.Find(CommandName))
	{
		UClass* CommandUClass = *CommandClassPtr;
		if (!CommandUClass) return;

		UCommandBase* Command = NewObject<UCommandBase>(this, CommandUClass);

		// Find target actor by first argument
		AActor* TargetActor = nullptr;
		if (Tokens.Num() > 0)
		{
			FString TargetName = Tokens[0];
			Tokens.RemoveAt(0);
			TargetActor = FindActorByName(GetWorld(), TargetName);

			if (!TargetActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("Actor not found: %s"), *TargetName);
				return;
			}
		}

		Command->ExecuteCommand(TargetActor, Tokens);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unknown command: %s"), *CommandName.ToString());
	}
}