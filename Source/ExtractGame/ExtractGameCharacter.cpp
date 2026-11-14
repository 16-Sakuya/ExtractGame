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
#include "PickupBase.h"
#include "Exfil.h"
#include "EngineUtils.h"//提供查找关卡中撤离点的TActorIterator
#include "GameFramework/PlayerController.h"//提供玩家控制器函数
#include "StashSaveGame.h"
#include "Kismet/GameplayStatics.h"//提供读取和写入存档的函数
#include "WeaponBase.h"
#include "Engine/World.h"//提供 SpawnActorDeferred / FinishSpawning 等世界相关函数
#include "Components/SkeletalMeshComponent.h"

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

void AExtractGameCharacter::BeginPlay() {
	Super::BeginPlay();

	hp = MaxHp;
	mp = MaxMp;
	OnHpChanged.Broadcast(hp, MaxHp);
	OnMpChanged.Broadcast(mp, MaxMp);

	if (UGameplayStatics::GetCurrentLevelName(this, true) == TEXT("L_Lobby")) { return; }
	
	UStashSaveGame* Save = Cast<UStashSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Stash"), 0));
	if (!Save || Save->CarryCnt.IsEmpty()) { return; }

	const TMap<FName, int32> CryItems = Save->CarryCnt;
	const int32 CryVal = Save->CryVal;

	Save->CarryCnt.Empty();
	Save->CryVal = 0;
	if (!UGameplayStatics::SaveGameToSlot(Save, TEXT("Stash"), 0)) { return; }

	LootCount = CryItems;
	TotalValue = CryVal;
}

void AExtractGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AExtractGameCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AExtractGameCharacter::DoJumpEnd);

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
	if (bInputLocked) { return; }   // 界面开着，直接吞掉这次起跳
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

	const TArray<APickupBase*> NearLoot = GetNearLoot();//读取刚筛选并排序的列表
	if (!NearLoot.IsEmpty()) {
		APickupBase* Loot = NearLoot[0];
		if (IsValid(Loot)) {//检查物品仍有效
			const FName LtName = Loot->GetPickupId();
			const int32 LtVal = Loot->GetPickupValue();

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

		const FString SlotName = TEXT("Stash");//指定仓库存档的名称
		const int32 UserIndex = 0;//指定本机玩家的存档编号。以后读取仓库时必须使用相同的名称和编号
		UStashSaveGame* Sth = Cast<UStashSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));//从刚才指定的仓库存档读取数据
		if (Sth == nullptr) {
			Sth = Cast<UStashSaveGame>(UGameplayStatics::CreateSaveGameObject(UStashSaveGame::StaticClass()));//如果没有存档就新建一个
		}
		//检查仓库对象是否创建成功，如果失败就打印错误日志并返回
		if (Sth == nullptr) {
			UE_LOG(LogExtractGame, Error, TEXT("无法创建仓库存档对象"));
			return;
		}

		for (const TPair<FName, int32>& Lt : LootCount) {
			int32& StCnt = Sth->StashCnt.FindOrAdd(Lt.Key);
			StCnt += Lt.Value;//本次数量加到仓库原有数量上
		}

		//把更新后的仓库写入同一个存档位置
		if (!UGameplayStatics::SaveGameToSlot(Sth, SlotName, UserIndex)) {
			UE_LOG(LogExtractGame, Error, TEXT("无法保存仓库存档"));
			return;
		}

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
TArray<APickupBase*> AExtractGameCharacter::GetNearLoot() const{
	TArray<APickupBase*> Res;//存放找到的可拾取物
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

	for (TActorIterator<APickupBase> It(GetWorld()); It; ++It) {
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
	Res.Sort([PlayerPos](const APickupBase& A, const APickupBase& B) {
		const float dstA = FVector::DistSquared(PlayerPos, A.GetActorLocation());
		const float dstB = FVector::DistSquared(PlayerPos, B.GetActorLocation());

		if (dstA == dstB) { return A.GetUniqueID() < B.GetUniqueID(); }
		return dstA < dstB;
		});

	return Res;
}

//丢弃物品
bool AExtractGameCharacter::DropALoot(FName ItemId, TSubclassOf<APickupBase> LootClass, int64 Value) {
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

	APickupBase* DroppedLoot = GetWorld()->SpawnActorDeferred<APickupBase>(//先创建 Actor，暂不完成它的生成
		LootClass.Get(), SpawnTrsf, nullptr, this, //使用数据表指定的战利品蓝图，不指定拥有者，角色作为生成者
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn//允许在指定位置创建；是否与附近物体重叠要在游戏中检查
	);
	if (!DroppedLoot) { return false; }//创建失败就退出，背包暂不扣物品

	DroppedLoot->InitDroppedPickup(ItemId, static_cast<int32>(Value));//在物体进入场景前写入物品 ID 和价值
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

//实现开关函数
void AExtractGameCharacter::SetInputLocked(bool bLocked)
{
	bInputLocked = bLocked;      // 记住当前状态
	// 如果是刚锁上
	if (bInputLocked){StopJumping(); }          // 把正在进行的跳跃立刻掐断，避免角色带着跳跃速度飞出去}
}

void AExtractGameCharacter::ApplyDamage(int32 Amount)
{
	if (Amount <= 0 || bDead) { return; }                              // 非法输入或已死亡直接忽略
	hp = FMath::Clamp(hp - Amount, 0, MaxHp);                 // 扣血并夹在 0..MaxHp
	OnHpChanged.Broadcast(hp, MaxHp);                         // 通知所有监听者（UI）
	// 血空了触发死亡钩子
	if (hp <= 0) { 
		bDead = true;
		OnDead();
	}
}

void AExtractGameCharacter::Heal(int32 Amount)
{
	if (Amount <= 0 || bDead) { return; }
	hp = FMath::Clamp(hp + Amount, 0, MaxHp);
	OnHpChanged.Broadcast(hp, MaxHp);
}

bool AExtractGameCharacter::ConsumeMp(int32 Amount)
{
	if (Amount <= 0 || mp < Amount || bDead) { return false; }         // 蓝不够就失败，不改数据
	mp = FMath::Clamp(mp - Amount, 0, MaxMp);
	OnMpChanged.Broadcast(mp, MaxMp);
	return true;
}

void AExtractGameCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);   // 先让父类处理（它会广播委托等）

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		FallStartZ = GetActorLocation().Z;      // 起跳/踏空的那一刻，记下高度
		bFallTracked = true;                    // 标记"正在记录"
	}
}

void AExtractGameCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);                         // 父类会处理落地动画/委托

	if (!bFallTracked) { return; }               // 没记录过就跳过
	bFallTracked = false;                        // 一次坠落只结算一次

	const float FallDistance = FallStartZ - GetActorLocation().Z;   // 下落了多少厘米（正数）
	if (FallDistance <= FallDamageThreshold) { return; }            // 10 米以内不扣血

	const int32 Damage = FMath::FloorToInt((FallDistance - FallDamageThreshold) / FallDamagePerPoint);
	if (Damage > 0) { ApplyDamage(Damage); }     // 走统一的受伤入口（会广播血条、判断死亡）
}

AWeaponBase* AExtractGameCharacter::EquipWeaponClass(TSubclassOf<AWeaponBase> WeaponClass, int32 InAmmo, int32 InDurability)
{
	if (!WeaponClass || !GetMesh()) { return nullptr; }   // 没类或没骨骼网格，直接放弃

	UnequipWeapon();                                      // 先把旧武器销毁

	// ① 延迟生成：先创建对象，但【不执行 BeginPlay】
	AWeaponBase* NewWeapon = GetWorld()->SpawnActorDeferred<AWeaponBase>(
		WeaponClass,                      // 要生成的具体武器蓝图
		GetActorTransform(),              // 生成位置（其实马上会被挂点覆盖）
		this,                             // Owner
		this,                             // Instigator
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);  // 不做碰撞检测，避免生成失败

	if (!NewWeapon) { return nullptr; }

	// ② 在 BeginPlay 之前打上"我是被装备的"标记
	//    注意：bSettleOnBeginPlay 是 APickupBase 的 protected 成员，这里不能直接改；
	//    bEquippedOnSpawn = true 之后，AWeaponBase::BeginPlay 内部会自己把它置 false
	NewWeapon->bEquippedOnSpawn = true;

	// ③ 先挂到手上（此时 BeginPlay 还没跑，武器不会先在世界里闪一下）
	NewWeapon->AttachToComponent(
		GetMesh(),                                          // 角色的骨骼网格
		FAttachmentTransformRules::SnapToTargetIncludingScale,  // 跟随插槽的位移/旋转/缩放
		NewWeapon->AttachSocketName);                       // 插槽名（默认 hand_r）

	NewWeapon->InitStats(InAmmo, InDurability);           // 写入 DT 里的弹药/耐久

	// ④ 完成生成：BeginPlay 执行，因为标记已设，会走装备态
	NewWeapon->FinishSpawning(GetActorTransform());

	CurrentWeapon = NewWeapon;
	return NewWeapon;
}

void AExtractGameCharacter::UnequipWeapon()
{
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
}

bool AExtractGameCharacter::ConsumeLoot(FName ItemId, int32 Count)
{
	int32* Cnt = LootCount.Find(ItemId);          // 找这一行
	if (!Cnt || *Cnt <= 0 || Count <= 0) { return false; }   // 没有/非法

	*Cnt -= Count;                                 // 扣数量
	if (*Cnt <= 0) { LootCount.Remove(ItemId); }   // 扣到 0 就删行
	return true;
}