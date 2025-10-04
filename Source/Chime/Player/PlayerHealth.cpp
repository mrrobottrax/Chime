#include "PlayerHealth.h"
#include "Managers/GameManager.h"

UPlayerHealth::UPlayerHealth()
{
	
}

void UPlayerHealth::BeginPlay()
{
	Super::BeginPlay();

	bIsDead = false;
}

void UPlayerHealth::Respawn()
{
	bIsDead = false;

	// Reset player position using singleton
	if (UWorld* World = GetWorld())
	{
		if (UGameManager* GM = World->GetGameInstance()->GetSubsystem<UGameManager>())
		{
			FVector SpawnLocation = GM->GetCurrentPlayerSpawn();

			if (AActor* Owner = GetOwner())
			{
				Owner->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::None);
			}
		}
	}
}

void UPlayerHealth::Die()
{
	bIsDead = true;

	// Move this elsewhere later when we have UI or some shit
	Respawn();
}