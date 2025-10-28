#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameManager.generated.h"


UCLASS()
class CHIME_API UGameManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection)override;

	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	FVector GetCurrentPlayerSpawn() const { return CurrentPlayerSpawn; }

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void UpdatePlayerSpawn(const FVector& NewLocation) { CurrentPlayerSpawn = NewLocation; }

private:

	FVector CurrentPlayerSpawn;
};
