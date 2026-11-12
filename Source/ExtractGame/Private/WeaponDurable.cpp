// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponDurable.h"

AWeaponDurable::AWeaponDurable()
{
	bUseAmmo = false;         // 不消耗弹药
	bUseDurability = true;    // 有耐久
	MaxDurability = 100.f;
	Durability = MaxDurability;
}
