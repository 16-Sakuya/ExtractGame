#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class AExtractGameCharacter;
class UStaticMeshComponent;

// 开火要额外消耗什么资源（必须写在 UCLASS 之前）
UENUM(BlueprintType)
enum class EWeaponCostType : uint8
{
	None UMETA(DisplayName = "不消耗"),
	Mp   UMETA(DisplayName = "消耗蓝量"),
	Hp   UMETA(DisplayName = "消耗生命")
};

UCLASS(Abstract)
class EXTRACTGAME_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool Reload();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnFired();

	// 装备时由角色调用：写入初始弹药/耐久
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void InitStats(int32 InAmmo, float InDurability);

	// 挂到角色身上用的插槽名
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocketName = TEXT("hand_r");

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

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

	AExtractGameCharacter* GetOwnerCharacter() const;
};