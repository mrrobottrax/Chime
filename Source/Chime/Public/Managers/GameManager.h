#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameManager.generated.h"


UCLASS()
class CHIME_API UGameManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	virtual void Initialize(FSubsystemCollectionBase& Collection)override;

	virtual void Deinitialize() override;

public:
	void LoadLevel(FString name);

private:
	UPROPERTY(BlueprintReadOnly, Category = "Spawning", meta = (AllowPrivateAccess = "true"))
	FVector CurrentPlayerSpawn;

	UFUNCTION()
	void OnPostLoadMap(UWorld* LoadedWorld);
	void CachePlayerStart(UWorld* World);
};
