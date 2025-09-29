#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "WindZone.generated.h"

UCLASS()

class AWindZone : public AActor
{
	GENERATED_BODY()

// Constructor
public:
	AWindZone();

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

// Box Component
protected:
	// Wind Zone Volume
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* WindVolume;

	// Wind Properties
	UPROPERTY(EditAnywhere, Category = "Wind Properties")
	FVector WindDir = FVector(0, 0, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Wind Properties")
	float WindStrength = 5000.0f;

	UFUNCTION()
	void BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TSet<AActor*> OverlappingActors;
};
