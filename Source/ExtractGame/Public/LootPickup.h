// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootPickup.generated.h"

//球体检测前向声明
class USphereComponent;
class UTextRenderComponent;
class UPrimitiveComponent;

//静态网格组件的前向声明
class UStaticMeshComponent;

UCLASS()
class EXTRACTGAME_API ALootPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALootPickup();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;

	//检测范围球体组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* DetectionSphere;

	//文本渲染组件，用于显示战利品名称
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UTextRenderComponent* Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")

	//战利品名称
	FName LootName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 LootValue;

	// 入场时是否先落地再固定：场景里摆得比地面高的战利品用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bSettleOnBeginPlay = true;

	// 是否由运行时"丢弃"生成。丢弃物有自己的 DroppedPhysics 流程，不走入场落地
	bool bDropped = false;

	// 当前是否正在入场落地
	bool bSettling = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//检测范围球体组件的重叠开始事件
	UFUNCTION()
	void OnDetectionBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
	//检测范围球体组件的重叠结束事件
	UFUNCTION()
	void OnDetectionEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	// 网格碰到东西时的回调
	UFUNCTION()
	void OnLootHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 结束落地：关物理并恢复碰撞
	void StopSettlePhysics();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//获取战利品名称
	UFUNCTION(BlueprintPure, Category = "Loot")
	FName GetLootName() const { return LootName; }
	//获取战利品价值
	int32 GetLootValue() const { return LootValue; }

	//丢弃战利品
	void DroppedLoot(FName InName, int32 InValue) {
		LootName = InName;
		LootValue = InValue;
		bDropped = true;      // 标记为运行时丢弃，跳过入场落地
	}

	//物理函数
	bool DroppedPhysics();

protected:
	bool DrpToStl = false;//记录该物品正在等待落稳
	float DrpPhysicsElapsed = 0.f;//记录开启物理后的时间，避免刚生成的那一帧就误判为休眠
};
