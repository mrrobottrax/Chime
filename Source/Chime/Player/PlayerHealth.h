#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealth.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHIME_API UPlayerHealth : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Constructor
	UPlayerHealth();

	void Respawn();

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void Die();

protected:
	virtual void BeginPlay() override;

	bool bIsDead;
};
