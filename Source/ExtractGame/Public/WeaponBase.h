#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"                 // 父类：落地物理、检测球、头顶文字都在这里
#include "WeaponBase.generated.h"

class AExtractGameCharacter;

// 开火要额外消耗什么资源
UENUM(BlueprintType)
enum class EWeaponCostType : uint8
{
	None UMETA(DisplayName = "不消耗"),
	Mp   UMETA(DisplayName = "消耗蓝量"),
	Hp   UMETA(DisplayName = "消耗生命")
};

UCLASS(Abstract)
class EXTRACTGAME_API AWeaponBase : public APickupBase
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// 尝试开火
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	// 换弹
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool Reload();

	// 开火成功的表现（蓝图实现）
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnFired();

	// 写入初始弹药/耐久（装备时由角色调用）
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void InitStats(int32 InAmmo, float InDurability);

	// 装备/卸下：装备时关物理、关碰撞、关检测球、藏文字
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetEquipped(bool bEquipped);

	// 生成时就是被装备的（装备流程用 SpawnActorDeferred 设置，避免先掉一下）
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bEquippedOnSpawn = false;

	// 挂到角色手上用的插槽名（角色装备武器时要读，所以放 public）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocketName = TEXT("hand_r");

protected:
	virtual void BeginPlay() override;

	// 头顶提示："武器ID(#价值)\n按[F]拾取"
	virtual FString GetPickupPrompt() const override;

	virtual FName GetPickupId() const override { return WeaponId; }
	virtual int32 GetPickupValue() const override { return WeaponValue; }
	virtual void ApplyPickupData(FName InId, int32 InValue) override;

	// 运行时的弹药/耐久读写（给 UI 用）
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetAmmo() const { return Ammo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetDurability() const { return Durability; }

	// ---- 身份 ----
	// 武器 ID：和 DT_Definition 的行名一致（比如 rifle01）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponId = NAME_None;

	// 价值（进背包时累加 CryVal 用）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int32 WeaponValue = 50;

	// ---- 弹药 ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	bool bUseAmmo = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 MaxAmmo = 30;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 Ammo = 30;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 AmmoPerShot = 1;

	// ---- 耐久 ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Durability")
	bool bUseDurability = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Durability")
	float MaxDurability = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Durability")
	float Durability = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Durability")
	float DurabilityPerShot = 1.f;

	// ---- 额外资源 ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Cost")
	EWeaponCostType CostType = EWeaponCostType::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Cost")
	int32 CostAmount = 0;

	// ---- 射击参数 ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire")
	float FireInterval = 0.15f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire")
	int32 DamagePerShot = 10;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire")
	float Range = 5000.f;

	float LastFireTime = -1000.f;

	// 当前是否处于装备状态
	bool bIsEquipped = false;

	AExtractGameCharacter* GetOwnerCharacter() const;
};
