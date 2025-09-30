#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "UWindVolumeComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CHIME_API UWindVolumeComponent : public UBoxComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Properties")
	float WindStrength = 5000.0f;
};