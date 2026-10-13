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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//获取战利品名称
	FName GetLootName() const { return LootName; }
	//获取战利品价值
	int32 GetLootValue() const { return LootValue; }

};
