#include "RotatorComponent.h"

URotatorComponent::URotatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URotatorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URotatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Rotate(DeltaTime);
}

void URotatorComponent::Rotate(float DeltaTime)
{
	FVector normalizedAxis = RotateAxis.GetSafeNormal();
	float DeltaAngle = RotateSpeed * DeltaTime;

	FQuat QuatRotation(normalizedAxis, FMath::DegreesToRadians(DeltaAngle));
	GetOwner()->AddActorLocalRotation(QuatRotation);
}

