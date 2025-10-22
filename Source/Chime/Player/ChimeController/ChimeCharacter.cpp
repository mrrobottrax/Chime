#include "ChimeCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include <Kismet/KismetMathLibrary.h>
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Managers/GameManager.h"
#include "Chime/LevelTools/InteractableBase/InteractableBase.h"
#include "Chime/Player/ChimeCamera/PlayerCameraComponent.h"
#include "DrawDebugHelpers.h"

#pragma region Movement Constants

// -- Walking / Ground Movement --
const float kMaxWalkSpeed = 640.0f;
const float kBreakingDecelerationWalking = 4000.0f;
const float kMaxAcceleration = 1000.0f;
const float kBrakingFrictionFactor = 1.0f;
const float kGroundFriction = 10.0f;
const float kAirControl = 1.0f;

// -- Jumping --
const float kDefaultJumpZVelocity = 700.0f;
const float kJumpMaxHoldTime = 0.25f;

// Coyote Time
const float kMaxCoyoteTime = 0.18f;

// -- Wall Jump --
const float kWallJumpTraceDistance = 50.0f;
const float kWallJumpTraceRadius = 10.0f;
const int kWallJumpAdjacentImpulse = 800;
const int kWallJumpVerticalImpulse = 900;
const float kWallJumpDelay = 0.4f;

// -- Gravity --
const float kDefaultGravityScale = 3.0f;
const float kMaxFallingZVel = -3600.0f;
const float kFallingLateralFriction = 0.1f;

// Wall falling clamps
const float kMaxWallZVel = -400.0f;
const float kMaxWallXYVel = 60.0f;

// Glide
const float kMaxGlideZVel = -170.0f;
const float kMaxWindVel = 3000.0f;// The maximum velocity while in a wind zone

// -- Ground Pound --
const int kGroundPoundImpulse = 1800;
const float kGroundPoundInputBuffer = 0.15f;

// -- Context Action --
const float kPokeTraceDistance = 130.0f;

#pragma endregion

AChimeCharacter::AChimeCharacter()
{
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	PlayerCamera = CreateDefaultSubobject<UPlayerCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	PlayerCamera->bUsePawnControlRotation = false;

	// Physics handle
	BeakPhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("BeakPhysicsHandle"));
	BeakComponent = CreateDefaultSubobject<USceneComponent>(TEXT("BeakComponent"));
	BeakComponent->SetupAttachment(RootComponent);

	// Bind overlaps
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AChimeCharacter::OnCharacterOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &AChimeCharacter::OnCharacterEndOverlap);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Movement constants
	MoveComp->MaxWalkSpeed = kMaxWalkSpeed;
	MoveComp->BrakingDecelerationWalking = kBreakingDecelerationWalking;
	MoveComp->MaxAcceleration = kMaxAcceleration;
	MoveComp->BrakingFrictionFactor = kBrakingFrictionFactor;
	MoveComp->GroundFriction = kGroundFriction;
	MoveComp->AirControl = kAirControl;

	// Jumping
	JumpMaxHoldTime = kJumpMaxHoldTime;
	MoveComp->JumpZVelocity = kDefaultJumpZVelocity;

	// Gravity / Falling
	MoveComp->GravityScale = kDefaultGravityScale;
	MoveComp->FallingLateralFriction = kFallingLateralFriction;

	// Misc
	MoveComp->bUseFlatBaseForFloorChecks = true;

	// Unreal assumes you made a jump when you enter the falling state.
	// This is a hacky fix that Unreal recomends officially! :D
	JumpMaxCount = 3;
}

void AChimeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set player spawn
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			UGameManager* gameManager = GI->GetSubsystem<UGameManager>();
			if (gameManager)
			{
				gameManager->UpdatePlayerSpawn(GetActorLocation());
			}
		}
	}
}

#pragma region Character Functions

	void AChimeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
	{
		// Set up action bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

			// Moving
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChimeCharacter::Move);

			// Looking
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChimeCharacter::Look);

			// Jumping
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AChimeCharacter::DoJumpStart);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AChimeCharacter::DoJumpEnd);

			// Crouching
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AChimeCharacter::DoCrouchStart);
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AChimeCharacter::DoCrouchEnd);

			// Context-ing?
			EnhancedInputComponent->BindAction(ContextAction, ETriggerEvent::Started, this, &AChimeCharacter::DoContextStart);
			EnhancedInputComponent->BindAction(ContextAction, ETriggerEvent::Completed, this, &AChimeCharacter::DoContextEnd);

			EnhancedInputComponent->BindAction(UseInteractable, ETriggerEvent::Started, this, &AChimeCharacter::DoUseInteractableStart);

			EnhancedInputComponent->BindAction(GearAction, ETriggerEvent::Started, this, &AChimeCharacter::DoGearDriver);
		}
	}

	void AChimeCharacter::Move(const FInputActionValue& Value)
	{
		InputDirection = Value.Get<FVector2D>();

		DoMove(InputDirection.X, InputDirection.Y);
	}

	void AChimeCharacter::Look(const FInputActionValue& Value)
	{
		FVector2D LookAxisVector = Value.Get<FVector2D>();

		DoLook(LookAxisVector.X, LookAxisVector.Y);
	}

	void AChimeCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
	{
		Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

		// Save time since we started falling
		if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling) 
			LastFallTime = GetWorld()->GetTimeSeconds();
	}

	void AChimeCharacter::NotifyJumpApex()
	{
		// Enter glide from double jump
		if (!bIsInputRestricted && bIsJumpPressed)
		{
			TryGlide();
		}
	}

	void AChimeCharacter::Landed(const FHitResult& Hit)
	{
		Super::Landed(Hit);

		bHasDoubleJumped = false;

		if (bIsGroundPounding)
			TryContextAction(FVector::DownVector);

		bIsGroundPounding = false;
		bIsGliding = false;
	}

	void AChimeCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
	{
		Super::EndPlay(EndPlayReason);

		// Reset timers
		GetWorld()->GetTimerManager().ClearTimer(PauseTimer);
		GetWorld()->GetTimerManager().ClearTimer(WallJumpTimer);
	}
#pragma endregion

#pragma region Input

	void AChimeCharacter::DoMove(float Right, float Forward)
	{
		if (bIsInputRestricted || bIsWallJumping || bIsGroundPounding)
			return;

		if (GetController() != nullptr)
		{
			// find out which way is forward
			const FRotator Rotation = GetController()->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

			// get right vector 
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// add movement 
			AddMovementInput(ForwardDirection, Forward);
			AddMovementInput(RightDirection, Right);
		}
	}

	void AChimeCharacter::DoLook(float Yaw, float Pitch)
	{
		if (GetController() != nullptr)
		{
			AddControllerYawInput(Yaw);
			AddControllerPitchInput(Pitch);
		}
	}

	void AChimeCharacter::DoJumpStart()
	{
		bIsJumpPressed = true;

		if (bIsInputRestricted)
			return;

		// Try glide
		TryGlide();

		if (bIsGliding || bIsGroundPounding)
			return;

		// Try normal jump
		TryJump();
	}

	void AChimeCharacter::DoJumpEnd()
	{
		bIsJumpPressed = false;

		StopJumping();

		// End glide if space released
		if (!bIsInputRestricted && bIsGliding)
		{
			EndGlide();
		}
	}

	void AChimeCharacter::DoCrouchStart()
	{
		bIsCrouched = true;

		if (bIsInputRestricted || bIsGroundPounding)
			return;

		// Ground pound
		if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling) 
		{
			DoGroundPound();
			return;
		}
	}

	void AChimeCharacter::DoCrouchEnd()
	{
		bIsCrouched = false;
	}

	void AChimeCharacter::DoContextStart()
	{
		isActionPressed = true;

		if (bIsInputRestricted || bIsWallJumping || bIsGroundPounding)
			return;

		TryContextAction(GetActorForwardVector());
	}

	void AChimeCharacter::DoContextEnd()
	{
		isActionPressed = false;
	}

	void AChimeCharacter::DoUseInteractableStart()
	{
		UE_LOG(LogTemp, Warning, TEXT("Try Interaction"));

		// Exit gear control mode
		if (CurrentContextAction == EContextAction::ECS_ControllingGear) 
		{
			CurrentContextAction = EContextAction::ECS_None;
			return;
		}

		// Enter gear control mode
		if (IsValid((AActor*)CurrentInteractable))
		{
			CurrentContextAction = EContextAction::ECS_ControllingGear;
			UE_LOG(LogTemp, Warning, TEXT("Working"));
		}
	}

	void AChimeCharacter::DoGearDriver(const FInputActionValue& Value)
	{
		// Gears!!!
	}
#pragma endregion

#pragma region Unreal Callbacks

	void AChimeCharacter::Tick(float DeltaTime)
	{
		Super::Tick(DeltaTime);

		bool bPrevOnWall = bIsOnWall;
		bIsOnWall = CheckForWall(false);

		// Kill all velocity if wall was just mounted
		if (!bPrevOnWall && bIsOnWall)
			StartWallSlide();

		// Handle velocity clamping
		HandleVelocity();

		if (CurrentContextAction == EContextAction::ECS_Dragging) 
		{
			if (BeakPhysicsHandle && BeakPhysicsHandle->GrabbedComponent)
			{
				// Snap the object to the beak
				FVector BeakLoc = BeakComponent->GetComponentLocation();
				FRotator BeakRot = BeakComponent->GetComponentRotation();
				BeakPhysicsHandle->SetTargetLocationAndRotation(BeakLoc, BeakRot);
			}
		}
	}

#pragma endregion

#pragma region Actions

	void AChimeCharacter::TryJump()
	{
		GetCharacterMovement()->bNotifyApex = true;

		if (!bIsGliding) 
		{
			if (GetCharacterMovement()->IsFalling())
			{
				FHitResult Hit;
				if (CheckForWall(true, &Hit))
				{
					// Rotate the character to face away from the wall
					FRotator WallOrientation = Hit.ImpactNormal.ToOrientationRotator();
					WallOrientation.Pitch = 0.0f;
					WallOrientation.Roll = 0.0f;
					SetActorRotation(WallOrientation);

					// Apply a launch impulse to the player
					const FVector WallJumpImpulse =
						(Hit.ImpactNormal* kWallJumpAdjacentImpulse) +
							(FVector::UpVector * kWallJumpVerticalImpulse);

					// Break from wall poke
					if (CurrentContextAction == EContextAction::ECS_Poking)
						UnstickFromSurface();

						LaunchCharacter(WallJumpImpulse, true, true);

						bIsWallJumping = true;
						GetWorld()->GetTimerManager().SetTimer(
							WallJumpTimer,
							this,
							&AChimeCharacter::OnWallJumpEnd,
							kWallJumpDelay,
							false
						);

						UE_LOG(LogTemp, Warning, TEXT("Wall Jump"));
				}
				else if (!bIsWallJumping)
				{
					// Check for coyote time
					if (GetWorld()->GetTimeSeconds() - LastFallTime < kMaxCoyoteTime)
					{
						Jump();
						UE_LOG(LogTemp, Warning, TEXT("Coyote Jump"));
					}
					else if (!bHasDoubleJumped)
					{
						bHasDoubleJumped = true;
						Jump();
						UE_LOG(LogTemp, Warning, TEXT("Double Jump"));
					}
				}
			}
			else
			{
				// Ground jump
				Jump();

				UE_LOG(LogTemp, Warning, TEXT("Jump"));
			}
		}
	}

	void AChimeCharacter::StartWallSlide()
	{
		GetCharacterMovement()->StopMovementImmediately();

		bIsGliding = false;
		UE_LOG(LogTemp, Warning, TEXT("Enter wall slide"));

		// To Do
		// Particles, Animations, and sound go here
	}

	void AChimeCharacter::OnWallJumpEnd()
	{
		bIsWallJumping = false;
	}

	void AChimeCharacter::TryGlide()
	{
		if (GetCharacterMovement()->IsFalling() && (bIsGroundPounding || bHasDoubleJumped) &&
			!bIsGliding && !bIsOnWall && !bIsWallJumping)
		{
			// Kill downwards velocity
			bIsGroundPounding = false;
			FVector Vel = GetCharacterMovement()->Velocity;
			Vel.Z = 0;
			GetCharacterMovement()->Velocity = Vel;

			if (bIsInWind)
				GetCharacterMovement()->GravityScale = 0;

			// Start glide
			bIsGliding = true;

			UE_LOG(LogTemp, Warning, TEXT("Start glide"));
		}
	}

	void AChimeCharacter::EndGlide()
	{
		bIsGliding = false;
		GetCharacterMovement()->GravityScale = kDefaultGravityScale;

		UE_LOG(LogTemp, Warning, TEXT("End glide"));
	}

	void AChimeCharacter::OnEnterWind(AWindZone* windZone)
	{
		if (!IsValid(windZone))
			return;

		CurrentWindZone = windZone;
		bIsInWind = true;

		if (bIsGliding)
			GetCharacterMovement()->GravityScale = 0;

		UE_LOG(LogTemp, Warning, TEXT("Player entered wind"));
	}

	void AChimeCharacter::OnExitWind(AWindZone* windZone)
	{
		if (IsValid(CurrentWindZone) && CurrentWindZone == windZone)
		{
			bIsInWind = false;
			CurrentWindZone = nullptr;
			GetCharacterMovement()->GravityScale = kDefaultGravityScale;
			UE_LOG(LogTemp, Warning, TEXT("Player exited wind"));
		}
	}

	void AChimeCharacter::DoGroundPound()
	{
		bIsGroundPounding = true;
		bIsGliding = false;
		bIsOnWall = false;

		// Pause movement and get downwards impulse
		const FVector PoundImpulse = FVector::DownVector * kGroundPoundImpulse;
		PauseMovement(kGroundPoundInputBuffer, PoundImpulse);

		UE_LOG(LogTemp, Warning, TEXT("Ground pound"));
	}

	void AChimeCharacter::TryContextAction(FVector traceDirection) 
	{
		FVector traceStart = GetActorLocation();
		const FVector traceEnd = traceStart + (traceDirection * kPokeTraceDistance);

		FHitResult hitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);// Ignore self
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.bTraceComplex = false;

		bool bIsActorHit = GetWorld()->LineTraceSingleByChannel(
			hitResult,
			traceStart,
			traceEnd,
			ECC_Visibility,
			QueryParams
		);

		DrawDebugLine(
			GetWorld(),
			traceStart,
			traceEnd,
			FColor::Red,
			false,
			2.0f,
			0,
			2.0f
		);

		// Try to undo current context state if possible
		switch (CurrentContextAction)
		{
			case EContextAction::ECS_Poking:
				UnstickFromSurface();
				return;

			case EContextAction::ECS_Dragging:
				DropDraggedObject();
				return;
		}

		if (bIsActorHit && IsValid(hitResult.PhysMaterial.Get()))
		{
			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());

			switch (SurfaceType)
			{
				case EPhysicalSurface::SurfaceType1:
					// Soft surface
					if (hitResult.GetComponent()->IsSimulatingPhysics()) 
					{
						// Pickup soft rigidbodies
						if (!bIsGroundPounding)
							StartDragObject(hitResult);
					}
					else
						// Stick into soft solid surfaces (even when ground pounding)
						StickToSurface(hitResult);
					break;

				case EPhysicalSurface::SurfaceType2:
					// Make sparks and shit
					break;
			}
		}
	}

	void AChimeCharacter::StickToSurface(FHitResult hitResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("Poke into surface"));
		CurrentContextAction = EContextAction::ECS_Poking;

		FVector WallNormal = hitResult.ImpactNormal;

		// Face wall
		FRotator TargetRotation = (-WallNormal).Rotation();
		SetActorRotation(TargetRotation);

		// Snap to wall surface
		FVector hitLocation = hitResult.Location;
		FVector dirToPlayer = hitLocation - GetActorLocation();
		FVector newLocation = hitLocation + (WallNormal * GetCapsuleComponent()->GetUnscaledCapsuleRadius());
		SetActorLocation(newLocation);

		// Follow surface transfrom
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->GravityScale = 0;
		GetCharacterMovement()->StopActiveMovement();
		this->AttachToActor(hitResult.GetActor(), FAttachmentTransformRules::KeepWorldTransform);
	}

	void AChimeCharacter::UnstickFromSurface()
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->GravityScale = kDefaultGravityScale;
		this->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		CurrentContextAction = EContextAction::ECS_None;
	}

	void AChimeCharacter::StartDragObject(FHitResult hitResult)
	{
		CurrentContextAction = EContextAction::ECS_Dragging;

		UPrimitiveComponent* HitComp = hitResult.GetComponent();
		if (HitComp && HitComp->IsSimulatingPhysics())
		{
			BeakPhysicsHandle->GrabComponentAtLocationWithRotation(
				HitComp,
				NAME_None,
				hitResult.ImpactPoint,
				HitComp->GetComponentRotation()
			);

			// Snap the object to the beak
			FVector BeakLoc = BeakComponent->GetComponentLocation();
			FRotator BeakRot = BeakComponent->GetComponentRotation();
			BeakPhysicsHandle->SetTargetLocationAndRotation(BeakLoc, BeakRot);
		}
	}

	void AChimeCharacter::DropDraggedObject()
	{
		if (BeakPhysicsHandle && BeakPhysicsHandle->GrabbedComponent)
		{
			BeakPhysicsHandle->ReleaseComponent();
		}

		CurrentContextAction = EContextAction::ECS_None;
	}
#pragma endregion

#pragma region Physics

	void AChimeCharacter::PauseMovement(float duration, FVector resumeVelocity) 
	{
		// Kill input 
		bIsInputRestricted = true;

		// Store desired velocity before pause
		StoredResumeVel = resumeVelocity;

		GetCharacterMovement()->DisableMovement();
		GetWorldTimerManager().SetTimer(PauseTimer, this, &AChimeCharacter::OnResumeMovement, duration, false);
	}

	void AChimeCharacter::OnResumeMovement()
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// Launch player then reset desired velocity
		LaunchCharacter(StoredResumeVel, true, true);
		StoredResumeVel = FVector::ZeroVector;

		// Restore movement 
		bIsInputRestricted = false;
	}

	bool AChimeCharacter::CheckForWall(bool isJumpIgnored, FHitResult* outHit)
	{
		if (bIsGroundPounding || (!isJumpIgnored && bIsJumpPressed) || !GetCharacterMovement()->IsFalling())
			return false;

		// Cast capsule forward
		FVector traceStart = GetActorLocation(); 
		const FVector traceEnd = traceStart + (GetActorForwardVector() * kWallJumpTraceDistance); 
		const FCollisionShape traceShape = FCollisionShape::MakeSphere(kWallJumpTraceRadius);

		// Ignore self
		FHitResult hitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		bool isWallHit = GetWorld()->SweepSingleByChannel
		(
			hitResult,
			traceStart, 
			traceEnd, 
			FQuat(), 
			ECollisionChannel::ECC_Visibility, 
			traceShape, 
			QueryParams
		);

		if (outHit) 
			*outHit = hitResult;

		return isWallHit;
	}

	void AChimeCharacter::HandleVelocity()
	{
		FVector Vel = GetCharacterMovement()->Velocity;

		if (Vel.Z < kMaxFallingZVel)
		{
			// Regular falling clamp
			Vel.Z = kMaxFallingZVel;
			GetCharacterMovement()->Velocity = Vel;
		}
		else if (bIsOnWall)
		{
			// Clamp wall slide vel
			Vel.X = FMath::Clamp(Vel.X, -kMaxWallXYVel, kMaxWallXYVel);
			Vel.Y = FMath::Clamp(Vel.Y, -kMaxWallXYVel, kMaxWallXYVel);

			if (Vel.Z < kMaxWallZVel)
			{
				Vel.Z = kMaxWallZVel;
				GetCharacterMovement()->Velocity = Vel;
			}
		}
		else if (bIsGliding)
		{
			// Wind velocity clamp
			if (bIsInWind)
			{
				FVector currentVel = Vel;
				FVector clampedVel = UKismetMathLibrary::ClampVectorSize(currentVel, 0, kMaxWindVel);
				GetCharacterMovement()->Velocity = clampedVel;
				return;
			}

			// Standard glid clamp
			if (Vel.Z < kMaxGlideZVel) 
			{
				Vel.Z = kMaxGlideZVel;
				GetCharacterMovement()->Velocity = Vel;
			}
		}
	}

	void AChimeCharacter::OnCharacterOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
	{
		if (!IsValid(OtherActor))
			return;

		AActor* ParentActor = OtherActor->GetAttachParentActor();
		if (IsValid(ParentActor) && ParentActor->IsA<AInteractableBase>())
		{
			UE_LOG(LogTemp, Warning, TEXT("Entered interactable trigger"));
			CurrentInteractable = Cast<AInteractableBase>(ParentActor);
		}

	}

	void AChimeCharacter::OnCharacterEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
	{
		if (!IsValid(OtherActor))
			return;

		AActor* ParentActor = OtherActor->GetAttachParentActor();
		if (IsValid(ParentActor) && ParentActor == CurrentInteractable)
		{
			CurrentInteractable = nullptr;
			UE_LOG(LogTemp, Warning, TEXT("Exited interactable trigger"));
		}
	}

#pragma endregion