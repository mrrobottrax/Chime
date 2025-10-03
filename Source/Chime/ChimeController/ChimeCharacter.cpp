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

#pragma region Movement Constants

// -- Walking / Ground Movement --
const float MaxWalkSpeed = 640.0f;
const float BreakingDecelerationWalking = 4000.0f;
const float MaxAcceleration = 1000.0f;
const float BrakingFrictionFactor = 1.0f;
const float GroundFriction = 10.0f;
const float AirControl = 1.0f;

// -- Jumping --
const float DefaultJumpZVelocity = 900.0f;
const float JumpMaxHoldTime = 0.25f;

// Coyote Time
const float MaxCoyoteTime = 0.18f;

// -- Wall Jump --
const float WallJumpTraceDistance = 50.0f;
const float WallJumpTraceRadius = 10.0f;
const int WallJumpAdjacentImpulse = 800;
const int WallJumpVerticalImpulse = 900;
const float WallJumpDelay = 0.4f;

// -- Gravity --
const float DefaultGravityScale = 2.5f;
const float MaxFallingZVel = -3600.0f;
const float FallingLateralFriction = 0.1f;

// Wall falling clamps
const float MaxWallZVel = -400.0f;
const float MaxWallXYVel = 60.0f;

// Glide
const float MaxGlideZVel = -170.0f;
const float MaxWindVel = 1000.0f;// The maximum velocity while in a wind zone

// -- Ground Pound --
const int GroundPoundImpulse = 1800;
const float GroundPoundInputBuffer = 0.15f;

#pragma endregion


AChimeCharacter::AChimeCharacter()
{
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Movement constants
	MoveComp->MaxWalkSpeed = MaxWalkSpeed;
	MoveComp->BrakingDecelerationWalking = BreakingDecelerationWalking;
	MoveComp->MaxAcceleration = MaxAcceleration;
	MoveComp->BrakingFrictionFactor = BrakingFrictionFactor;
	MoveComp->GroundFriction = GroundFriction;
	MoveComp->AirControl = AirControl;

	// Jumping
	JumpMaxHoldTime = JumpMaxHoldTime;
	MoveComp->JumpZVelocity = DefaultJumpZVelocity;

	// Gravity / Falling
	MoveComp->GravityScale = DefaultGravityScale;
	MoveComp->FallingLateralFriction = FallingLateralFriction;

	// Misc
	MoveComp->bUseFlatBaseForFloorChecks = true;

	// Unreal assumes you made a jump when you enter the falling state.
	// This is a hacky fix that Unreal recomends officially! :D
	JumpMaxCount = 3;
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
			EnhancedInputComponent->BindAction(ContextAction, ETriggerEvent::Started, this, &AChimeCharacter::DoContext);
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

	void AChimeCharacter::DoContext()
	{
		// Poke input goes here

		/*
		if (bIsMovementRestricted || bIsOnWall)
			return;
		*/
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
						(Hit.ImpactNormal * WallJumpAdjacentImpulse) +
						(FVector::UpVector * WallJumpVerticalImpulse);

					LaunchCharacter(WallJumpImpulse, true, true);

					bIsWallJumping = true;
					GetWorld()->GetTimerManager().SetTimer(
						WallJumpTimer,
						this,
						&AChimeCharacter::OnWallJumpEnd,
						WallJumpDelay,
						false
					);

					UE_LOG(LogTemp, Warning, TEXT("Wall Jump"));
				}
				else if (!bIsWallJumping)
				{
					// Check for coyote time
					if (GetWorld()->GetTimeSeconds() - LastFallTime < MaxCoyoteTime)
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

			// Start glide
			bIsGliding = true;

			UE_LOG(LogTemp, Warning, TEXT("Start glide"));
		}
	}

	void AChimeCharacter::EndGlide()
	{
		bIsGliding = false;
		GetCharacterMovement()->GravityScale = DefaultGravityScale;

		UE_LOG(LogTemp, Warning, TEXT("End glide"));
	}

	void AChimeCharacter::OnEnterWind()
	{
		bIsInWind = true;

		if (bIsGliding) 
			GetCharacterMovement()->GravityScale = 0;

		UE_LOG(LogTemp, Warning, TEXT("Player entered wind"));
	}

	void AChimeCharacter::OnExitWind()
	{
		bIsInWind = false;

		GetCharacterMovement()->GravityScale = DefaultGravityScale;

		UE_LOG(LogTemp, Warning, TEXT("Player exited wind"));
	}

	void AChimeCharacter::DoGroundPound()
	{
		bIsGroundPounding = true;
		bIsGliding = false;
		bIsOnWall = false;

		// Pause movement and get downwards impulse
		const FVector PoundImpulse = FVector::DownVector * GroundPoundImpulse;
		PauseMovement(GroundPoundInputBuffer, PoundImpulse);

		UE_LOG(LogTemp, Warning, TEXT("Ground pound"));
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

		// Ignore self
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		// Cast capsule forward
		FHitResult hitResult; const 
		FVector Location = GetActorLocation(); 
		const FVector TraceEnd = Location + (GetActorForwardVector() * WallJumpTraceDistance); 
		const FCollisionShape TraceShape = FCollisionShape::MakeSphere(WallJumpTraceRadius);

		bool isWallHit = GetWorld()->SweepSingleByChannel
		(
			hitResult,
			Location, 
			TraceEnd, 
			FQuat(), 
			ECollisionChannel::ECC_Visibility, 
			TraceShape, 
			QueryParams
		);

		if (outHit) 
			*outHit = hitResult;

		return isWallHit;
	}

	void AChimeCharacter::HandleVelocity()
	{
		FVector Vel = GetCharacterMovement()->Velocity;

		if (Vel.Z < MaxFallingZVel)
		{
			// Regular falling clamp
			Vel.Z = MaxFallingZVel;
			GetCharacterMovement()->Velocity = Vel;
		}
		else if (bIsOnWall)
		{
			// Clamp wall slide vel
			Vel.X = FMath::Clamp(Vel.X, -MaxWallXYVel, MaxWallXYVel);
			Vel.Y = FMath::Clamp(Vel.Y, -MaxWallXYVel, MaxWallXYVel);

			if (Vel.Z < MaxWallZVel)
			{
				Vel.Z = MaxWallZVel;
				GetCharacterMovement()->Velocity = Vel;
			}
		}
		else if (bIsGliding)
		{
			// Wind velocity clamp
			if (bIsInWind)
			{
				FVector currentVel = Vel;
				FVector clampedVel = UKismetMathLibrary::ClampVectorSize(currentVel, 0, MaxWindVel);
				GetCharacterMovement()->Velocity = clampedVel;
				return;
			}

			// Standard glid clamp
			if (Vel.Z < MaxGlideZVel) 
			{
				Vel.Z = MaxGlideZVel;
				GetCharacterMovement()->Velocity = Vel;
			}
		}
	}

#pragma endregion