#include "Managers/GameManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/Level.h"

void UGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGameManager::OnPostLoadMap);
}

void UGameManager::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

void UGameManager::OnPostLoadMap(UWorld* LoadedWorld)
{
	CachePlayerStart(LoadedWorld);
}

void UGameManager::LoadLevel(FString name)
{
	
}

void UGameManager::CachePlayerStart(UWorld* World)
{
	if (!World) return;

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		CurrentPlayerSpawn = It->GetActorTransform().GetLocation();
		UE_LOG(LogTemp, Warning, TEXT("Cached PlayerStart at %s"), *CurrentPlayerSpawn.ToString());
		return;
	}
}
