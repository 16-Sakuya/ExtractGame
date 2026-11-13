// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"          // 父类：可拾取物公共基类
#include "LootPickup.generated.h"

UCLASS()
class EXTRACTGAME_API ALootPickup : public APickupBase
{
	GENERATED_BODY()

public:
	ALootPickup();

	// 获取战利品名称 / 价值
	UFUNCTION(BlueprintPure, Category = "Loot")
	FName GetLootName() const { return LootName; }

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetLootValue() const { return LootValue; }

	// 运行时"丢弃"生成时写入数据
	void DroppedLoot(FName InName, int32 InValue)
	{
		LootName = InName;
		LootValue = InValue;
		bDropped = true;      // 标记为丢弃物，跳过入场落地
	}

protected:
	// 头顶提示："名称(#价值)\n按[F]拾取"
	virtual FString GetPickupPrompt() const override;

	virtual FName GetPickupId() const override { return LootName; }
	virtual int32 GetPickupValue() const override { return LootValue; }
	virtual void ApplyPickupData(FName InId, int32 InValue) override;

	//战利品名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FName LootName = NAME_None;

	//战利品价值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 LootValue = 0;
};
