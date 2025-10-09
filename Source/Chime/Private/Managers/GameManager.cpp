#include "Managers/GameManager.h"
#include "Engine/World.h"

void UGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Warning, TEXT("UGameManager Initialized"));
}

void UGameManager::Deinitialize()
{
	Super::Deinitialize();
}
