#include "PlayerHealth.h"

UPlayerHealth::UPlayerHealth()
{
	
}

void UPlayerHealth::BeginPlay()
{
	Super::BeginPlay();

	Respawn();
}

void UPlayerHealth::Respawn()
{
	bIsDead = false;

	// To Do:
	// Reset player position using singleton
}

void UPlayerHealth::Die()
{
	bIsDead = false;
}