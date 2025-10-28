#include "Teleport_Command.h"
#include "GameFramework/Character.h"

void UTeleport_Command::ExecuteCommand_Implementation(AActor* Executor, const TArray<FString>& Args)
{
	if (Args.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Usage: tp <x> <y> <z>"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(Executor);
	if (!Character) return;

	float X = FCString::Atof(*Args[0]);
	float Y = FCString::Atof(*Args[1]);
	float Z = FCString::Atof(*Args[2]);

	FVector NewLocation(X, Y, Z);
	Character->SetActorLocation(NewLocation);

	UE_LOG(LogTemp, Log, TEXT("Teleported to %s"), *NewLocation.ToString());
}