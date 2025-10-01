#include "MoverComponent.h"
#include "Math/UnrealMath.h"

UMoverComponent::UMoverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMoverComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	StartLocation = Owner->GetActorLocation();
	FinalLocation = StartLocation + EndLocation;
	IsMovingToFinalLocation = true;
	MoveSpeed = (StartLocation - FinalLocation).Length() / MoveDuration;
}

void UMoverComponent::UnPause()
{
	IsPaused = false;
}

void UMoverComponent::Move(float deltaTime)
{
	if (!IsPaused)
	{
		FVector CurrentLocation = GetOwner()->GetActorLocation();

		// Lerp to target
		FVector targetLocation = IsMovingToFinalLocation ? FinalLocation : StartLocation;
		FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, targetLocation, deltaTime, MoveSpeed);

		GetOwner()->SetActorLocation(NewLocation);

		// Flip direction if the target was reached
		IsAtTargetLocation = CurrentLocation.Equals(targetLocation, 0.2f);
		if (IsAtTargetLocation)
		{
			IsMovingToFinalLocation = !IsMovingToFinalLocation;
			IsPaused = PauseAtTarget;
		}
	}
}

void UMoverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Move(DeltaTime);
}