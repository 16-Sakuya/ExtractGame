#include "WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "ExtractGameCharacter.h"

AWeaponBase::AWeaponBase()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

AExtractGameCharacter* AWeaponBase::GetOwnerCharacter() const
{
	return Cast<AExtractGameCharacter>(GetOwner());
}

bool AWeaponBase::TryFire()
{
	AExtractGameCharacter* WeaponOwner = GetOwnerCharacter();
	if (!WeaponOwner) { return false; }

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < FireInterval) { return false; }

	if (bUseDurability && Durability <= 0.f) { return false; }
	if (bUseAmmo && Ammo < AmmoPerShot) { return false; }

	if (CostType == EWeaponCostType::Mp && !WeaponOwner->ConsumeMp(CostAmount)) { return false; }
	if (CostType == EWeaponCostType::Hp && WeaponOwner->GetHp() < CostAmount) { return false; }

	if (bUseAmmo) { Ammo -= AmmoPerShot; }
	if (bUseDurability) { Durability = FMath::Max(0.f, Durability - DurabilityPerShot); }
	if (CostType == EWeaponCostType::Hp) { WeaponOwner->ApplyDamage(CostAmount); }

	LastFireTime = Now;
	OnFired();
	return true;
}

bool AWeaponBase::Reload()
{
	if (!bUseAmmo) { return false; }
	Ammo = MaxAmmo;
	return true;
}

void AWeaponBase::InitStats(int32 InAmmo, float InDurability)
{
	if (bUseAmmo) { Ammo = FMath::Clamp(InAmmo, 0, MaxAmmo); }          // 只有耗弹武器才写弹药
	if (bUseDurability) { Durability = FMath::Clamp(InDurability, 0.f, MaxDurability); }
}