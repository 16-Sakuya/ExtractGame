// 可拾取物的公共基类：网格 + 检测球 + 头顶文字 + 落地物理
// 战利品（ALootPickup）和武器（AWeaponBase）都继承它，避免两份重复的落地/检测逻辑
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

class USphereComponent;
class UTextRenderComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class EXTRACTGAME_API APickupBase : public AActor
{
	GENERATED_BODY()

public:
	APickupBase();

	virtual void Tick(float DeltaTime) override;

	// 开启物理模拟，让它掉下来；返回是否成功启动
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	bool DroppedPhysics();

	// 当前是否正在"落地中"
	UFUNCTION(BlueprintPure, Category = "Pickup")
	bool IsSettling() const { return bSettling; }

	// 物品 ID：子类各自决定从哪个字段取
	UFUNCTION(BlueprintPure, Category = "Pickup")
	virtual FName GetPickupId() const { return NAME_None; }

	// 物品价值：子类各自决定从哪个字段取
	UFUNCTION(BlueprintPure, Category = "Pickup")
	virtual int32 GetPickupValue() const { return 0; }

	// 一次拾取能得到多少个（默认 1，子弹盒子类返回 30 之类的数量）
	UFUNCTION(BlueprintPure, Category = "Pickup")
	virtual int32 GetPickupCount() const { return 1; }

	// 运行时"丢弃"生成时调用：记录 ID/价值/数量，并标记为丢弃物（跳过入场落地）
	void InitDroppedPickup(FName InId, int32 InValue, int32 InCount = 1);

protected:
	virtual void BeginPlay() override;

	// 子类返回头顶提示文字；返回空字符串就表示不显示提示
	virtual FString GetPickupPrompt() const { return FString(); }

	// 子类把 ID/价值写进自己的字段
	virtual void ApplyPickupData(FName InId, int32 InValue) {}

	// 子类把"数量"写进自己的字段（丢弃时用；只有需要计数的子类才覆写）
	virtual void ApplyPickupCount(int32 InCount) {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;

	// 检测范围球体：只对 Pawn 产生重叠，用来触发"按 F 拾取"的提示
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* DetectionSphere;

	// 头顶文字
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UTextRenderComponent* Name;

	// 入场时是否先落地再固定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bSettleOnBeginPlay = true;

	// 是否强制使用引擎默认的文本材质。
	// 离线字体在自定义材质上很容易只画出"字形矩形"（整块实心），引擎默认材质是最稳的
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bUseDefaultTextMaterial = true;

	// 是否是运行时"丢弃"生成的（丢弃物自己开物理，不走入场落地）
	bool bDropped = false;

	// 当前是否正在入场落地
	bool bSettling = false;

	// 正在等待落稳
	bool DrpToStl = false;

	// 开启物理后经过的时间
	float DrpPhysicsElapsed = 0.f;

	UFUNCTION()
	void OnDetectionBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// 网格碰到东西
	UFUNCTION()
	void OnPickupHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 结束落地：关物理并恢复完整碰撞
	void StopSettlePhysics();
};
