// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponAmmo.h"

AWeaponAmmo::AWeaponAmmo()
{
	bUseAmmo = true;          // 消耗弹药
	bUseDurability = false;   // 没有耐久
	MaxAmmo = 30;
	Ammo = MaxAmmo;
}