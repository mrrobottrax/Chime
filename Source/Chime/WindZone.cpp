#include "WindZone.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "ChimeController/ChimeCharacter.h"

AWindZone::AWindZone()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WindVolume"));
	WindVolume->SetupAttachment(GetRootComponent());
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WindVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WindVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);
}

void AWindZone::BeginPlay()
{
	Super::BeginPlay();

	WindVolume->OnComponentBeginOverlap.AddDynamic(this, &AWindZone::BeginOverlap);
	WindVolume->OnComponentEndOverlap.AddDynamic(this, &AWindZone::EndOverlap);
}

void AWindZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector normalizedWindDir = WindDir.GetSafeNormal();

	// Add forces to each actor
	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor)) continue;

		FVector WindForce = normalizedWindDir * WindStrength * DeltaTime;

		// Apply to physics objects
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent())) 
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddForce(WindForce);
				continue;
			}
		}
		else if (AChimeCharacter* Player = Cast<AChimeCharacter>(Actor))
		{
			if (Player->bIsGliding)
			{
				Player->GetCharacterMovement()->AddInputVector(WindForce);
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

