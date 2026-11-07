// Fill out your copyright notice in the Description page of Project Settings.

// 大厅仓库与携带清单之间的搬运工具。全部是静态函数，蓝图里随取随用
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"   // 父类：蓝图函数库
#include "LootStorage.generated.h"             // UHT 生成的头，必须放最后一个 include

class UStashSaveGame;                          // 只用到指针，前向声明就够，省一次 include

UCLASS()
class EXTRACTGAME_API ULootStorage : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 读存档；读不到就返回一个新建的空存档对象（不写盘）
	UFUNCTION(BlueprintCallable, Category = "LootStorage")
	static UStashSaveGame* LoadStash();

	// 仓库 -> 背包。返回实际搬了几件（0 表示没搬成）
	UFUNCTION(BlueprintCallable, Category = "LootStorage")
	static int32 MoveToCarry(FName ItemId, int32 Count = 1, int32 UnitValue = 0);

	// 背包 -> 仓库。返回值同上
	UFUNCTION(BlueprintCallable, Category = "LootStorage")
	static int32 MoveToStash(FName ItemId, int32 Count = 1, int32 UnitValue = 0);

	// 按物品名升序排出仓库里的所有物品 ID，给 UI 用（避免每次刷新行序跳动）
	UFUNCTION(BlueprintCallable, Category = "LootStorage")
	static TArray<FName> GetSortedStashIds(UStashSaveGame* Save);

	// 同上，取携带清单的
	UFUNCTION(BlueprintCallable, Category = "LootStorage")
	static TArray<FName> GetSortedCarryIds(UStashSaveGame* Save);
};