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
#include "GameFramework/PlayerController.h"//提供玩家控制器函数

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
//f按键映射
void AExtractGameCharacter::Interact_f(const FInputActionValue&)
{
	if (hasOverlap) { return;  }
	UE_LOG(LogExtractGame, Display, TEXT("f按下"));
	if (GetController() == nullptr || GetWorld() == nullptr) { return;  }

	const TArray<ALootPickup*> NearLoot = GetNearLoot();//读取刚筛选并排序的列表
	if (!NearLoot.IsEmpty()) {
		ALootPickup* Loot = NearLoot[0];
		if (IsValid(Loot)) {//检查物品仍有效
			const FName LtName = Loot->GetLootName();
			const int32 LtVal = Loot->GetLootValue();

			if (!Loot->Destroy()) { return; }

			int32& Cnt = LootCount.FindOrAdd(LtName);
			++Cnt;
			TotalValue += LtVal;

			UE_LOG(LogExtractGame, Display, TEXT("拾取战利品：%s	数量：%d	价值：#%d"), *LtName.ToString(), Cnt, LtVal);
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

//显示周围战利品
TArray<ALootPickup*> AExtractGameCharacter::GetNearLoot() const{
	TArray<ALootPickup*> Res;//存放找到的战利品
	if (!GetWorld()) { return Res; }//关卡不可用时返回空列表

	const APlayerController* PC = Cast<APlayerController>(GetController());//获取玩家控制器
	if (!PC) { return Res; }//没有玩家控制器时返回空列表

	//获取屏幕宽度和高度。尺寸无效返回空列表
	int32 ScrWidth = 0, ScrHeight = 0;
	PC->GetViewportSize(ScrWidth, ScrHeight);
	if (ScrWidth <= 0 || ScrHeight <= 0) { return Res; }

	//视角位置和方向
	FVector ViewLct;
	FRotator ViewRtt;
	PC->GetPlayerViewPoint(ViewLct, ViewRtt);//取得摄像机视角
	const FCollisionQueryParams SightParams(SCENE_QUERY_STAT(NearLootSight), false, this);//设置射线检测，并忽略角色自身

	for (TActorIterator<ALootPickup> It(GetWorld()); It; ++It) {
		const FVector LootToPlayer = It->GetActorLocation() - GetActorLocation();//战利品相对角色的位置
		const float HrztReach = GetCapsuleComponent()->GetScaledCapsuleRadius() + 50.f;//角色胶囊体半径再向外延申50cm
		const float VtcReach = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 50.f;//限制上下距离防止把不同楼层的战利品算入

		//检查战利品距离
		if(LootToPlayer.SizeSquared2D() > FMath::Square(HrztReach) || FMath::Abs(LootToPlayer.Z) > VtcReach) { continue; }

		const FVector LootCenter = It->GetRootComponent()->Bounds.Origin;//取得战利品网格的中心

		//这段把物品位置投影到屏幕；投影失败或落在画面外，就跳过该物品
		FVector2D ScrPos;
		if (!PC->ProjectWorldLocationToScreen(LootCenter, ScrPos) ||
			ScrPos.X < 0.f || ScrPos.X >= ScrWidth ||
			ScrPos.Y < 0.f || ScrPos.Y >= ScrHeight
			) {continue; }

		FHitResult SightHit;//保存射线碰到的物体，如果没碰到战利品，或先碰到了墙，就跳过它
		if(!GetWorld()->LineTraceSingleByChannel(SightHit, ViewLct, LootCenter, ECC_Visibility, SightParams) ||
			SightHit.GetActor() != *It) {continue; }

		Res.Add(*It);
	}

	const FVector PlayerPos = GetActorLocation();
	//按距离排列，让最近的物品排在前面
	Res.Sort([PlayerPos](const ALootPickup& A, const ALootPickup& B) {
		const float dstA = FVector::DistSquared(PlayerPos, A.GetActorLocation());
		const float dstB = FVector::DistSquared(PlayerPos, B.GetActorLocation());

		if (dstA == dstB) { return A.GetUniqueID() < B.GetUniqueID(); }
		return dstA < dstB;
		});

	return Res;
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