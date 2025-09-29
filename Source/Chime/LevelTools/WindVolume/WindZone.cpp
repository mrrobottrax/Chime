#include "WindZone.h"
#include "UWindVolumeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Chime/ChimeController/ChimeCharacter.h"

AWindZone::AWindZone()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	// Create Wind Volume
	WindVolume = CreateDefaultSubobject<UWindVolumeComponent>(TEXT("WindVolume"));
	WindVolume->SetupAttachment(RootComponent);

	// Collision setup
	WindVolume->SetCollisionProfileName("Trigger");
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WindVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WindVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);
}

void AWindZone::BeginPlay()
{
	Super::BeginPlay();

	if (WindVolume)
	{
		WindVolume->OnComponentBeginOverlap.AddDynamic(this, &AWindZone::BeginOverlap);
		WindVolume->OnComponentEndOverlap.AddDynamic(this, &AWindZone::EndOverlap);
	}
}

void AWindZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!WindVolume) return;

	FVector WindForce = WindVolume->WindDir.GetSafeNormal() * WindVolume->WindStrength;

	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor)) continue;

		// Apply to physics objects
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddForce(WindForce);
				continue;
			}
		}

		// Apply to gliding player
		if (AChimeCharacter* Player = Cast<AChimeCharacter>(Actor))
		{
			if (Player->bIsGliding)
			{
				Player->GetCharacterMovement()->Velocity += WindForce * DeltaTime;
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

