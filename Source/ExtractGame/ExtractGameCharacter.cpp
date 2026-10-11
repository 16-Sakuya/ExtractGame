// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExtractGameCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ExtractGame.h"
#include "LootPickup.h"

AExtractGameCharacter::AExtractGameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.0f;//弹簧臂长度
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AExtractGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExtractGameCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AExtractGameCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AExtractGameCharacter::Look);

		//f按键映射
		EnhancedInputComponent->BindAction(InteractAction_f, ETriggerEvent::Started, this, &AExtractGameCharacter::Interact_f);
	}
	else
	{
		UE_LOG(LogExtractGame, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AExtractGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AExtractGameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AExtractGameCharacter::DoMove(float Right, float Forward)
{
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

void AExtractGameCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AExtractGameCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AExtractGameCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AExtractGameCharacter::Interact_f(const FInputActionValue&)
{
	UE_LOG(LogExtractGame, Display, TEXT("f按下"));
	if (GetController() == nullptr || GetWorld() == nullptr) { return;  }

	FVector ViewLocation;// 玩家视角位置
	FRotator ViewRotation;// 玩家视角旋转
	GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);// 获取玩家视角位置和旋转
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * 500.f;// 计算射线终点位置

	FHitResult Hit;// 射线检测结果
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractTrace), false, this);// 创建射线检测参数。设置射线检测名称，不使用复杂碰撞检测，忽略自身

	if (GetWorld()->LineTraceSingleByChannel(
		Hit,				// 射线检测结果
		ViewLocation,		// 射线起点位置
		TraceEnd,			// 射线终点位置
		ECC_Visibility,		// 指定射线检测通道，物体需要在这个通道上阻挡射线，检测才能命中
		QueryParams)		// 把刚才创建的检测参数传进去，并结束这次射线调用。若射线碰到了物体，函数返回 true
		) {
		if (ALootPickup* Loot = Cast<ALootPickup>(Hit.GetActor())) {//获取命中的物体并尝试将其转换为 ALootPickup
			UE_LOG(LogExtractGame, Display, TEXT("拾取战利品"));
			Loot->Destroy();//销毁战利品
		}
	}
}