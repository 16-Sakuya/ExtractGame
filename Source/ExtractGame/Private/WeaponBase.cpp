#include "WeaponBase.h"

#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "ExtractGameCharacter.h"

AWeaponBase::AWeaponBase()
{
	// 网格、检测球、头顶文字都由 APickupBase 构造函数创建
}

void AWeaponBase::BeginPlay()
{
	// 装备流程里生成的武器：跳过入场落地，直接进装备态
	if (bEquippedOnSpawn) { bSettleOnBeginPlay = false; }

	Super::BeginPlay();

	if (bEquippedOnSpawn) { SetEquipped(true); }
}

AExtractGameCharacter* AWeaponBase::GetOwnerCharacter() const
{
	return Cast<AExtractGameCharacter>(GetOwner());
}

FString AWeaponBase::GetPickupPrompt() const
{
	return FString::Printf(TEXT("%s(#%d)\n按[F]拾取"), *WeaponId.ToString(), WeaponValue);
}

void AWeaponBase::ApplyPickupData(FName InId, int32 InValue)
{
	WeaponId = InId;
	WeaponValue = InValue;
}

void AWeaponBase::InitStats(int32 InAmmo, float InDurability)
{
	if (bUseAmmo) { Ammo = FMath::Clamp(InAmmo, 0, MaxAmmo); }
	if (bUseDurability) { Durability = FMath::Clamp(InDurability, 0.f, MaxDurability); }
}

void AWeaponBase::SetEquipped(bool bEquipped)
{
	bIsEquipped = bEquipped;

	if (MeshComponent)
	{
		MeshComponent->SetSimulatePhysics(false);
		MeshComponent->SetCollisionEnabled(bEquipped
			? ECollisionEnabled::NoCollision
			: ECollisionEnabled::QueryAndPhysics);
	}

	SetActorEnableCollision(!bEquipped);

	// 装备后不要再触发"按 F 拾取"，也不要显示头顶文字
	if (DetectionSphere) { DetectionSphere->SetGenerateOverlapEvents(!bEquipped); }
	if (Name) { Name->SetVisibility(false); }

	if (bEquipped)
	{
		DrpToStl = false;
		bSettling = false;
	}
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
