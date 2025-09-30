#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UWindVolumeComponent.h"
#include "WindZone.generated.h"

class UWindVolumeComponent;

UCLASS(Blueprintable)
class CHIME_API AWindZone : public AActor
{
	GENERATED_BODY()

public:
	AWindZone();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UWindVolumeComponent* WindVolume;

	UFUNCTION()
	void BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnConstruction(const FTransform& Transform);

private:
	TSet<AActor*> OverlappingActors;
};