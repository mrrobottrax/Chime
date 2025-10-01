#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoverComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHIME_API UMoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMoverComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void UnPause();

	virtual void Move(float deltaTime);

	UPROPERTY(EditAnywhere)
	FVector EndLocation;

	UPROPERTY(EditAnywhere)
	float MoveDuration;

	UPROPERTY(EditAnywhere)
	bool PauseAtTarget;

protected:

	// System
	FVector StartLocation;
	FVector FinalLocation;// The start + end in worldspace

	bool IsMovingToFinalLocation;
	bool IsAtTargetLocation;
	bool IsPaused;

	float MoveSpeed;
};
