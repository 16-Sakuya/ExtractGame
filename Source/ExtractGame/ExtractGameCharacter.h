// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ExtractGameCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class APickupBase;
struct FInputActionValue;
class AWeaponBase;

//"数值变了"的广播类型，两个参数分别是当前值和上限
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVitalChanged, int32, Current, int32, Max);

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AExtractGameCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction_f;

	//战利品清单表
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<FName, int32> LootCount;
	//总价值
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 TotalValue = 0;

	// 血量 / 蓝量
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	int32 hp = 100;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	int32 mp = 100;

	// 上限：EditDefaultsOnly 让它在蓝图类默认值里能改，运行时改不了
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	int32 MaxHp = 100;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	int32 MaxMp = 100;

	// 变化广播：BlueprintAssignable 让蓝图能"绑定事件"
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnVitalChanged OnHpChanged;
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnVitalChanged OnMpChanged;

	// 是否已经死亡：防止 OnDead 重复触发
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	bool bDead = false;

	// 界面打开时锁住角色输入：移动、跳跃、交互都不响应
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	bool bInputLocked = false;

	// 坠落伤害：起坠高度阈值（厘米）。10 米 = 1000 cm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float FallDamageThreshold = 500.f;

	// 坠落伤害：超过阈值后每多少厘米扣 1 点
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float FallDamagePerPoint = 10.f;

	//是否已撤离
	bool hasOverlap = false;

	// 当前武器
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeaponBase* CurrentWeapon = nullptr;

public:

	/** Constructor */
	AExtractGameCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 落地回调：算坠落高度、结算伤害
	virtual void Landed(const FHitResult& Hit) override;

	// 移动模式变化：记录进入下落瞬间的高度
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	//f按键映射
	void Interact_f(const FInputActionValue& Value);

	//覆盖开始游戏事件
	virtual void BeginPlay() override;

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	//撤离结算
	UFUNCTION(BlueprintImplementableEvent, Category = "Exfil")
	void ExfilSettle(int32 FinalValue, const FText& LootList);

	//丢弃物品
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropALoot(FName ItemId, TSubclassOf<APickupBase> LootClass, int64 Value);

	//拾取物品
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<APickupBase*> GetNearLoot() const;

	// 蓝图调用：true = 锁住，false = 解锁
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputLocked(bool bLocked);

	//受伤入口
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(int32 Amount);

	//回血入口
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(int32 Amount);

	//获取当前血量和蓝量
	UFUNCTION(BlueprintPure, Category = "Health")
	int32 GetHp() const { return hp; }
	UFUNCTION(BlueprintPure, Category = "Health")
	int32 GetMp() const { return mp; }

	//消耗蓝量入口
	UFUNCTION(BlueprintCallable, Category = "Health")
	bool ConsumeMp(int32 Amount);

	// 死亡时给蓝图一个钩子（倒地动画、回大厅、弹结算都行）
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")//C++ 只负责触发，具体做什么在蓝图里实现
	void OnDead();

	// 生成并装备一把武器，返回新武器（失败返回 nullptr）
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AWeaponBase* EquipWeaponClass(TSubclassOf<AWeaponBase> WeaponClass, int32 InAmmo, float InDurability);

	// 卸下当前武器（销毁）
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();

	// 获取当前武器（可能是 nullptr）
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	// 开始下落那一帧的世界 Z（厘米）
	float FallStartZ = 0.f;
	// 是否正在记录一次坠落
	bool bFallTracked = false;

};