// 子弹盒：拾取后一次给 AmmoCount 颗子弹，子弹作为背包里的一个堆叠物品（AmmoId）
#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "BulletBox.generated.h"

UCLASS()
class EXTRACTGAME_API ABulletBox : public APickupBase
{
	GENERATED_BODY()

public:
	ABulletBox();

	// 物品 ID：背包里这一行物品的名字（要和 DT_Definition 的行名一致）
	virtual FName GetPickupId() const override { return AmmoId; }

	// 每颗子弹的价值（结算 TotalValue = 价值 × 数量）
	virtual int32 GetPickupValue() const override { return ValuePerRound; }

	// 一次拾取能得到多少颗（默认整盒）
	virtual int32 GetPickupCount() const override { return AmmoCount; }

	// 蓝图里也能改数量（比如以后做 15 发的半盒）
	UFUNCTION(BlueprintCallable, Category = "Bullet")
	void SetAmmoCount(int32 InCount) { AmmoCount = FMath::Max(0, InCount); }

	UFUNCTION(BlueprintPure, Category = "Bullet")
	int32 GetAmmoCount() const { return AmmoCount; }

protected:
	// 头顶提示："子弹762×30\n按[F]拾取"
	virtual FString GetPickupPrompt() const override;

	// 丢弃时把 ID / 每颗价值写回自己的字段
	virtual void ApplyPickupData(FName InId, int32 InValue) override;

	// 丢弃时把"盒里剩多少颗"写回来
	virtual void ApplyPickupCount(int32 InCount) override;

	// 背包里这一行物品的 ID（和 DT_Definition 行名一致）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet")
	FName AmmoId = TEXT("子弹762");

	// 一盒多少颗
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet")
	int32 AmmoCount = 30;

	// 每颗的价值
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet")
	int32 ValuePerRound = 1;
};
