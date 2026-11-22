#include "BulletBox.h"

ABulletBox::ABulletBox()
{
	// 网格 / 检测球 / 头顶文字 都由 APickupBase 的构造函数创建，这里不用再加
}

FString ABulletBox::GetPickupPrompt() const
{
	// 例："子弹762×30\n按[F]拾取"
	return FString::Printf(TEXT("%s×%d\n按[F]拾取"), *AmmoId.ToString(), AmmoCount);
}

void ABulletBox::ApplyPickupData(FName InId, int32 InValue)
{
	if (!InId.IsNone()) { AmmoId = InId; }        // 丢弃时把原来的 ID 带回来
	ValuePerRound = FMath::Max(0, InValue);       // 每颗价值
}

void ABulletBox::ApplyPickupCount(int32 InCount)
{
	if (InCount > 0) { AmmoCount = InCount; }     // 丢弃 15 颗 → 地上盒子里就是 15 颗
}
