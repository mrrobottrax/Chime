#include "Managers/GameManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

void UGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Warning, TEXT("UGameManager Initialized"));

	// Bind to all world initializations
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UGameManager::OnWorldInit);

	// Attempt to cache spawn for current world if it already exists
	if (UWorld* World = GetWorld())
	{
		CachePlayerStart(World);
	}
}

void UGameManager::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	Super::Deinitialize();
}

void UGameManager::OnWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
	// Only cache for game worlds
	if (!World || !World->IsGameWorld())
		return;

	UE_LOG(LogTemp, Warning, TEXT("OnWorldInit called for world: %s"), *World->GetName());
	CachePlayerStart(World);
}

void UGameManager::CachePlayerStart(UWorld* World)
{
	if (!World) return;

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		CurrentPlayerSpawn = It->GetActorTransform().GetLocation();
		UE_LOG(LogTemp, Warning, TEXT("Initial PlayerStart cached at: %s"), *CurrentPlayerSpawn.ToString());
		return; // Only cache the first PlayerStart
	}

	UE_LOG(LogTemp, Warning, TEXT("No PlayerStart found in world: %s"), *World->GetName());
}
