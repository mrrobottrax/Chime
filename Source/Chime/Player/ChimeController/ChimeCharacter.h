#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Chime/LevelTools/WindVolume/WindZone.h"
#include "ChimeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

UCLASS(abstract)
class AChimeCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

// -- System vars -- 
protected:

	// -- Input --
	FVector2D InputDirection;
	bool bIsInputRestricted = false;

	// -- Physics --
	FTimerHandle PauseTimer;
	FVector StoredResumeVel;// Applied to player after movement pause

	// -- Jumping --
	bool bIsJumpPressed = false;
	float LastFallTime = 0.0f;// last system time the player started falling

	// Wall jump
	bool bIsOnWall = false;
	bool bIsWallJumping = false;// Used to kill double jump after wall jump
	FTimerHandle WallJumpTimer;

	// Double jump
	bool bHasDoubleJumped = false;

	// -- Crouching --
	bool bIsCrouching = false;
	bool bIsGroundPounding = false;

public: 
	// -- Gliding -- 
	bool bIsGliding = false;
	bool bIsInWind = false;
	AWindZone* CurrentWindZone = nullptr;

// Constructor
public:
	AChimeCharacter();

// Unreal Callbacks
public:

	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

// Input Actions
protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CrouchAction;

	/** Context Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ContextAction;

// Input handlers
public:

	/** Handles move inputs */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs*/
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCrouchStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCrouchEnd();

	/** Handles context defined inputs */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoContext();

// Actions
protected:

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void TryJump();

	void StartWallSlide();

	void OnWallJumpEnd();

	void TryGlide();

	void EndGlide();

	void DoGroundPound();

public:

	void OnEnterWind(AWindZone* windZone);

	void OnExitWind(AWindZone* windZone);


// Physics
protected:

	/** Checks for a wall in the direction the player is moving */
	virtual bool CheckForWall(bool isJumpIgnored, FHitResult* result = nullptr);

	/** Clamps the players z velocity based on falling state */
	virtual void HandleVelocity();

	/** Pauses the characters velocity and ignores input for a duration of time. 
	Once the duration has elapsed, the passed in velocity will be applied.*/
	void PauseMovement(float duration, FVector resumeVelocity);

	void OnResumeMovement();

protected:

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Called when the player character reaches the apex of their jump  */
	virtual void NotifyJumpApex() override;

	virtual void Landed(const FHitResult& Hit) override;

// Camera
public:

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
