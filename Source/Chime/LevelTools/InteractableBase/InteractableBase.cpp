#include "InteractableBase.h"

AInteractableBase::AInteractableBase()
{
	CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	InteractionLocation = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionLocation"));

	PrimaryActorTick.bCanEverTick = true;
}

void AInteractableBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AInteractableBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AInteractableBase::DoInteractionStart()
{

}

void AInteractableBase::DoInteractionEnd()
{

}

