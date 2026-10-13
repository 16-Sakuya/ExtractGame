// Fill out your copyright notice in the Description page of Project Settings.


#include "Exfil.h"

#include "Components/BoxComponent.h"

// Sets default values
AExfil::AExfil()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	ExfilArea = CreateDefaultSubobject<UBoxComponent>(TEXT("ExfilArea"));
	SetRootComponent(ExfilArea);
	ExfilArea->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	ExfilArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);//只进行碰撞查询而不阻挡
	ExfilArea->SetCollisionResponseToAllChannels(ECR_Ignore);//先忽略所有碰撞通道
	ExfilArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);//仅对角色产生的pawn通道产生重叠
	ExfilArea->SetGenerateOverlapEvents(true);//启用重叠检测

}

bool AExfil::isOverlap(const AActor* Actor) const
{
	return Actor != nullptr && ExfilArea->IsOverlappingActor(Actor);
}

// Called when the game starts or when spawned
void AExfil::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AExfil::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

