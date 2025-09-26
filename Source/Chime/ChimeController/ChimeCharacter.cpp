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

#pragma region Constant Variables

	// Coyote time
	const float MaxCoyoteTime = 0.18f;

	// Falling velocity
	const float MaxFallingZVel = -3600;
	const float MaxWallZVel = -400.f;
	const float MaxWallYVel = 60.f;// Horizontal wall movement
	const float MaxGlideZVel = -170.f;

	// Wall jump
	const float WallJumpTraceDistance = 50.0f;
	const float WallJumpTraceRadius = 7.0f;
	const int WallJumpAdjacentImpulse = 800;
	const int WallJumpVerticalImpulse = 900;
	const float DelayBetweenWallJumps = 0.4f;

	// Ground pound
	const int GroundPoundImpulse = 1800;
	const float GroundPoundInputBuffer = .15f;

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
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Set default gravity
	GetCharacterMovement()->GravityScale = 2.5f;

	// enable press and hold jump
	JumpMaxHoldTime = 0.25f;

	// Unreal  assumes you made a jump when you enter the falling state
	// This is a hacky fix that Unreal recomends officially :D
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
		if (bIsInputRestricted || bIsGroundPounding)
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

		if (bIsInputRestricted || bIsGroundPounding)
			return;

		// Try glide
		TryGlide();

		if (bIsGliding)
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
		bIsOnWall = CheckForWall();

		// Kill all velocity if wall was just mounted
		if (!bPrevOnWall && bIsOnWall)
			StartWallSlide();

		// Handle velocity clamping
		HandleFallingVelocity();
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
				if (CheckForWall(&Hit))
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

					bIsInputRestricted = true;
					GetWorld()->GetTimerManager().SetTimer(
						WallJumpTimer,
						this,
						&AChimeCharacter::ResetWallJump,
						DelayBetweenWallJumps,
						false
					);
				}
				else
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
			}
		}
	}

	void AChimeCharacter::ResetWallJump()
	{
		bIsInputRestricted = false;
	}

	void AChimeCharacter::TryGlide()
	{
		if (GetCharacterMovement()->IsFalling() && !bIsGliding &&
			bHasDoubleJumped && !bIsGroundPounding)
		{
			bIsGliding = true;
		}
	}

	void AChimeCharacter::EndGlide()
	{
		bIsGliding = false;
	}

	void AChimeCharacter::DoGroundPound() 
	{
		bIsGroundPounding = true;
		bIsGliding = false;
		bIsOnWall = false;

		// Pause movement and get downwards impulse
		const FVector PoundImpulse = FVector::DownVector * GroundPoundImpulse;
		PauseMovement(GroundPoundInputBuffer, PoundImpulse);
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
		GetWorldTimerManager().SetTimer(PauseTimer, this, &AChimeCharacter::ResumeMovement, duration, false);
	}

	void AChimeCharacter::ResumeMovement()
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// Launch player then reset desired velocity
		LaunchCharacter(StoredResumeVel, true, true);
		StoredResumeVel = FVector::ZeroVector;

		// Restore movement 
		bIsInputRestricted = false;
	}

	bool AChimeCharacter::CheckForWall(FHitResult* outHit)
	{
		if (bIsGroundPounding || !GetCharacterMovement()->IsFalling())
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


	void AChimeCharacter::HandleFallingVelocity()
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
			Vel.X = FMath::Clamp(Vel.X, -MaxWallYVel, MaxWallYVel);
			Vel.Y = FMath::Clamp(Vel.Y, -MaxWallYVel, MaxWallYVel);

			if (Vel.Z < MaxWallZVel)
			{
				Vel.Z = MaxWallZVel;
				GetCharacterMovement()->Velocity = Vel;
			}
		}
		else if (bIsGliding && Vel.Z < MaxGlideZVel)
		{
			// Gliding clamp
			Vel.Z = MaxGlideZVel;
			GetCharacterMovement()->Velocity = Vel;
		}
	}

	void AChimeCharacter::StartWallSlide() 
	{
		GetCharacterMovement()->StopMovementImmediately();

		bIsGliding = false;

		// To Do
		// Particles, Animations, and sound go here
	}

#pragma endregion