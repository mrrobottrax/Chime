#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableBase.generated.h"

UCLASS()
class CHIME_API AInteractableBase : public AActor
{
	GENERATED_BODY()
	
public :
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InteractionLocation;

public:
	AInteractableBase();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void DoInteractionStart();

	virtual void DoInteractionEnd();

};
