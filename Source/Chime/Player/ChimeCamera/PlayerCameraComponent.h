#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Chime/Player/ChimeController/ChimeCharacter.h"
#include "PlayerCameraComponent.generated.h"


UCLASS()
class CHIME_API UPlayerCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
	TSubclassOf<UCameraShakeBase> DeathShakeBP;

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void PlayDeathShake();

	void PlayShake(TSubclassOf<UCameraShakeBase> ShakeClass);

	AChimeCharacter* ChimeCharacter;
};
