// Fill out your copyright notice in the Description page of Project Settings.

#include "LootPickup.h"

ALootPickup::ALootPickup()
{
	// 网格、检测球、头顶文字都由 APickupBase 构造函数创建
}

FString ALootPickup::GetPickupPrompt() const
{
	return FString::Printf(TEXT("%s(#%d)\n按[F]拾取"), *LootName.ToString(), LootValue);
}

void ALootPickup::ApplyPickupData(FName InId, int32 InValue)
{
	LootName = InId;
	LootValue = InValue;
}
