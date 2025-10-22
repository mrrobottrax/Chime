#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHealth.generated.h"

// Public delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHIME_API UPlayerHealth : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Constructor
	UPlayerHealth();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeath OnDeath;

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void Respawn();

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void Die();


protected:
	virtual void BeginPlay() override;

	bool bIsDead;
};
