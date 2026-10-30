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
#include "Exfil.h"
#include "EngineUtils.h"//提供查找关卡中撤离点的TActorIterator

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
	if (hasOverlap) { return;  }
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
			const FName LootName = Loot->GetLootName();//获取战利品名称
			int32& Count = LootCount.FindOrAdd(LootName);//在战利品清单表中查找战利品名称，如果不存在则添加并返回引用
			Count++;//战利品数量加一
			TotalValue += Loot->GetLootValue();//战利品总价值加上战利品的价值
			UE_LOG(LogExtractGame, Display,
				TEXT("拾取战利品：%s，当前数量：%d, 当前总价值：#%d"),
				*LootName.ToString(),
				Count,
				TotalValue);//打印拾取战利品的名称和当前数量
			Loot->Destroy();//销毁战利品
			return;
		}
	}

	//撤离点交互
	for (TActorIterator<AExfil> Exfil(GetWorld()); Exfil; ++Exfil) {
		if (!Exfil->isOverlap(this)) { continue;  }

		hasOverlap = true;
		UE_LOG(LogExtractGame, Display, TEXT("撤离结算：#%d"), TotalValue);
		FString LootList;
		for (const TPair<FName, int32>& tLoot : LootCount) {
			UE_LOG(LogExtractGame, Display, TEXT("战利品：%s	数量：%d"), *tLoot.Key.ToString(), tLoot.Value);
			LootList += FString::Printf(TEXT("%20s	%d\n"), *tLoot.Key.ToString(), tLoot.Value);
		}
		if (LootList.IsEmpty()) { LootList = TEXT("未获取战利品"); }
		ExfilSettle(TotalValue, FText::FromString(LootList));
		return;
	}
}

//丢弃物品
bool AExtractGameCharacter::DropALoot(FName ItemId, TSubclassOf<ALootPickup> LootClass, int64 Value) {
	int32* count = LootCount.Find(ItemId);
	if (!count || *count <= 0 || !LootClass.Get() || Value<0 || Value > MAX_int32 || !GetWorld() ||!FollowCamera) { return false; }

	const FVector CmrLct = FollowCamera->GetComponentLocation();//相机位置
	const FVector DrpDir = FRotator(0.f, FollowCamera->GetComponentRotation().Yaw, 0.f).Vector();//获取相机朝向的水平方向
	const FVector DsrLct = GetActorLocation() + DrpDir * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 100.f) + FVector(0.f, 0.f, 50.f);//把生成目标放在角色碰撞胶囊前方 100 cm，并抬高 50 cm，让物品落下。这里改用角色作为位置起点，因为“相机前方 50 cm”与“角色前方”在当前第三人称视角下不是同一位置

	//射线检测忽略角色自身
	FCollisionQueryParams QryPrms;
	QryPrms.AddIgnoredActor(this);
	FHitResult FrtHit;//保存摄像机到目标位置之间的首次碰撞
	FVector SpawnLct = DsrLct;//默认使用DsrLct的位置

	//检查这50cm是否被地面挡住；返回首个阻挡命中
	if (GetWorld()->LineTraceSingleByChannel(FrtHit, CmrLct, DsrLct, ECC_Visibility, QryPrms)) {
		if (FrtHit.ImpactNormal.Z < 0.5f) { return false; }//判断命中的表面是否朝上；若撞到墙等侧面，返回失败，避免把战利品生成到墙里
		SpawnLct = FrtHit.ImpactPoint;//射线先碰到地面时，改用交点作为生成位置
	}
	const FTransform SpawnTrsf(FRotator::ZeroRotator, SpawnLct);//把最终位置交给现有的延迟生成代码

	ALootPickup* DroppedLoot = GetWorld()->SpawnActorDeferred<ALootPickup>(//先创建 Actor，暂不完成它的生成
		LootClass.Get(), SpawnTrsf, nullptr, this, //使用数据表指定的战利品蓝图，不指定拥有者，角色作为生成者
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn//允许在指定位置创建；是否与附近物体重叠要在游戏中检查
	);
	if (!DroppedLoot) { return false; }//创建失败就退出，背包暂不扣物品

	DroppedLoot->DroppedLoot(ItemId, static_cast<int32>(Value));//在物体进入场景前写入物品 ID 和价值
	DroppedLoot->FinishSpawning(SpawnTrsf);//完成生成。UE 的延迟生成流程要求调用这一步

	if (!DroppedLoot->DroppedPhysics()) {//尝试让刚生成的网格开启碰撞、物理模拟和重力
		DroppedLoot->Destroy();//清理无法正常掉落的 Actor
		return false;
	}

	--(*count);
	TotalValue -= static_cast<int32>(Value);
	if (*count == 0) { LootCount.Remove(ItemId); }
	return true;
}