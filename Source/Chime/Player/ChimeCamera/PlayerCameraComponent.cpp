#include "PlayerCameraComponent.h"
#include "Chime/Player/PlayerHealth.h"
#include "Chime/Player/ChimeController/ChimeController.h"
#include "Chime/Player/ChimeController/ChimeCharacter.h"

void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find health component on owner, then subscribe to death event.
	AActor* Owner = GetOwner();
	ChimeCharacter = Cast<AChimeCharacter>(Owner);

	if (IsValid(ChimeCharacter))
	{
		if (UPlayerHealth* Health = ChimeCharacter->FindComponentByClass<UPlayerHealth>())
		{
			UE_LOG(LogTemp, Warning, TEXT("Entered interactable trigger %s"), *ChimeCharacter->GetName());
			Health->OnDeath.AddDynamic(this, &UPlayerCameraComponent::PlayDeathShake);
		}
	}
}

void UPlayerCameraComponent::PlayDeathShake()
{
	UE_LOG(LogTemp, Warning, TEXT("Play Shake"));
	PlayShake(DeathShakeBP);
}

void UPlayerCameraComponent::PlayShake(TSubclassOf<UCameraShakeBase> ShakeClass)
{
	if (!IsValid(ShakeClass) || !IsValid(ChimeCharacter))
		return;

	if (AChimeController* Controller = Cast<AChimeController>(ChimeCharacter->GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Death Shake"));
		Controller->ClientStartCameraShake(ShakeClass);
	}
}