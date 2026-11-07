// Fill out your copyright notice in the Description page of Project Settings.

#include "LootStorage.h"

#include "ExtractGame.h"              // LogExtractGame 日志分类
#include "StashSaveGame.h"            // UStashSaveGame 的完整定义（要用到它的三个成员）
#include "Kismet/GameplayStatics.h"   // LoadGameFromSlot / SaveGameToSlot / CreateSaveGameObject

namespace
{
	// 存档槽名和玩家索引全局只在这里写一次。
	// 你其它地方（Character、WBP_Stash）用的是同名同索引，所以能读到同一份数据
	const TCHAR* const GStashSlotName = TEXT("Stash");
	constexpr int32 GStashUserIndex = 0;

	// 取某个物品的数量；表里没有就返回 0，不会因为找不到而崩
	int32 CountOf(const TMap<FName, int32>& Items, FName ItemId)
	{
		if (const int32* Found = Items.Find(ItemId))
		{
			return *Found;
		}
		return 0;
	}

	// 从表里扣掉 Amount 个；扣到 0 就把这一整行删掉，
	// 这样 UI 不会出现「数量 0」的僵尸行
	void ConsumeFrom(TMap<FName, int32>& Items, FName ItemId, int32 Amount)
	{
		int32& Cnt = Items.FindOrAdd(ItemId);   // 拿到引用，改它等于改表里的值
		Cnt -= Amount;
		if (Cnt <= 0)
		{
			Items.Remove(ItemId);
		}
	}

	// 真正搬运数量的核心。只操作两张表，不碰存档对象，
	// 所以读起来简单，也方便以后单独测试
	int32 MoveBetween(TMap<FName, int32>& From, TMap<FName, int32>& To, FName ItemId, int32 Count)
	{
		if (ItemId.IsNone() || Count <= 0) { return 0; }   // 非法输入直接不干活

		const int32 Have = CountOf(From, ItemId);           // 来源现有多少件
		const int32 Moved = FMath::Clamp(Count, 0, Have);   // 请求量钳到现有量——「右键整堆」就靠这里，传个很大的数即可
		if (Moved <= 0) { return 0; }                       // 一件都搬不动就返回

		ConsumeFrom(From, ItemId, Moved);      // 来源扣掉
		To.FindOrAdd(ItemId) += Moved;         // 目标累加（同类物品合并成一行）
		return Moved;
	}

	// 写盘。失败打 Error 日志并返回 false，让上层把这次操作当成没发生
	bool WriteStash(UStashSaveGame* Save)
	{
		if (!Save) { return false; }
		if (!UGameplayStatics::SaveGameToSlot(Save, GStashSlotName, GStashUserIndex))
		{
			UE_LOG(LogExtractGame, Error, TEXT("LootStorage: 无法保存仓库存档"));
			return false;
		}
		return true;
	}

	// 把 Map 的键取出来按名字升序排好
	TArray<FName> SortedKeysOf(const TMap<FName, int32>& Items)
	{
		TArray<FName> Keys;
		Items.GetKeys(Keys);                                            // 拿到所有物品 ID
		Keys.Sort([](const FName& A, const FName& B) {                   // 排序规则由这个 lambda 决定
			return A.LexicalLess(B);                                    // 按字母升序，保证每次刷新顺序一致
			});
		return Keys;
	}
}

UStashSaveGame* ULootStorage::LoadStash()
{
	// 先试着读盘。第一次玩或文件损坏时会失败，那就新建一个空存档对象返回；
	// 注意这里故意不写盘，只有真发生搬运时才写，避免无意义地覆盖用户文件
	if (UStashSaveGame* Loaded = Cast<UStashSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GStashSlotName, GStashUserIndex)))
	{
		return Loaded;
	}
	return Cast<UStashSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UStashSaveGame::StaticClass()));
}

int32 ULootStorage::MoveToCarry(FName ItemId, int32 Count, int32 UnitValue)
{
	UStashSaveGame* Save = LoadStash();      // 每次都从磁盘重读：两边 UI 不会互相覆盖
	if (!Save) { return 0; }

	const int32 Moved = MoveBetween(Save->StashCnt, Save->CarryCnt, ItemId, Count);
	if (Moved <= 0) { return 0; }            // 没搬成就不写盘，省一次磁盘操作

	if (UnitValue > 0)
	{
		// CryVal 语义 = 携带清单总价值。进图时 BeginPlay 会拿它当 TotalValue 的起点
		Save->CryVal += Moved * UnitValue;
	}

	if (!WriteStash(Save)) { return 0; }     // 写盘失败就当没搬：磁盘没变，UI 也不会刷新，不会出现假数据
	return Moved;
}

int32 ULootStorage::MoveToStash(FName ItemId, int32 Count, int32 UnitValue)
{
	UStashSaveGame* Save = LoadStash();
	if (!Save) { return 0; }

	const int32 Moved = MoveBetween(Save->CarryCnt, Save->StashCnt, ItemId, Count);  // 方向和上面反过来
	if (Moved <= 0) { return 0; }

	if (UnitValue > 0)
	{
		// 反向搬回时价值要退回，并防止算成负数
		Save->CryVal = FMath::Max(0, Save->CryVal - Moved * UnitValue);
	}

	if (!WriteStash(Save)) { return 0; }
	return Moved;
}

TArray<FName> ULootStorage::GetSortedStashIds(UStashSaveGame* Save)
{
	return Save ? SortedKeysOf(Save->StashCnt) : TArray<FName>();   // 空指针安全
}

TArray<FName> ULootStorage::GetSortedCarryIds(UStashSaveGame* Save)
{
	return Save ? SortedKeysOf(Save->CarryCnt) : TArray<FName>();
}