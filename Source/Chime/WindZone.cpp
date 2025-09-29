#include "WindZone.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"

AWindZone::AWindZone()
{
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneComponent;
	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TestCollision"));
	WindVolume->SetBoxExtent(FVector(5000.0, 5000.0, 5000.0));
	WindVolume->SetupAttachment(GetRootComponent());
	WindVolume->SetGenerateOverlapEvents(true);
	WindVolume->OnComponentBeginOverlap.AddDynamic(this, &AWindZone::BeginOverlap);
	WindVolume->OnComponentEndOverlap.AddDynamic(this, &AWindZone::EndOverlap);

}

void AWindZone::BeginPlay()
{
	Super::BeginPlay();
}

void AWindZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector normalizedWindDir = WindDir.GetSafeNormal();

	// Add forces to each actor
	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor)) continue;

		// Apply to physics objects
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent())) 
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddForce(normalizedWindDir * WindStrength);
				continue;
			}
		}
	}
}

void AWindZone::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor))
	{
		OverlappingActors.Add(OtherActor);
	}
}

void AWindZone::EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor)) 
	{
		OverlappingActors.Remove(OtherActor);
	}
}

