#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RotatorComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHIME_API URotatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URotatorComponent();

protected:
	virtual void BeginPlay() override;

	void Rotate(float DeltaTime);

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	FVector RotateAxis = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere)
	float RotateSpeed = 50.f;
};
