#include "WindZone.h"
#include "UWindVolumeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ArrowComponent.h"
#include "Chime/Player/ChimeController/ChimeCharacter.h"

const float dampeningForce = 2.5f;

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

	// Arrow gizmo
	UArrowComponent* Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("WindArrow"));
	Arrow->SetupAttachment(RootComponent);
	Arrow->ArrowSize = 3.0f;
	Arrow->ArrowColor = FColor::Blue;
	Arrow->bIsEditorOnly = true;
}

void AWindZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (UArrowComponent* Arrow = FindComponentByClass<UArrowComponent>())
	{
		FVector Up = GetActorUpVector();
		Arrow->SetWorldRotation(Up.Rotation());
	}
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


	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor)) continue;

		FVector windForce = GetActorUpVector() * WindVolume->WindStrength;

		// Apply to physics objects
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddForce(windForce);
				continue;
			}
		}

		// Apply to gliding player
		if (AChimeCharacter* Player = Cast<AChimeCharacter>(Actor))
		{
			if (Player->bIsGliding)
			{
				FVector playerVel = Player->GetCharacterMovement()->Velocity;

				// Smoothly lerp the players entry velocity to match the winds velocity
				FVector newVelocity = FMath::Lerp(playerVel, windForce, dampeningForce * DeltaTime);
			
				Player->GetCharacterMovement()->Velocity = newVelocity;
			}
		}
	}
}

void AWindZone::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor)) return;

	OverlappingActors.Add(OtherActor);

	if (AChimeCharacter* Player = Cast<AChimeCharacter>(OtherActor))
		Player->OnEnterWind();
}

void AWindZone::EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsValid(OtherActor)) return;

	OverlappingActors.Remove(OtherActor);

	if (AChimeCharacter* Player = Cast<AChimeCharacter>(OtherActor))
		Player->OnExitWind();
}
