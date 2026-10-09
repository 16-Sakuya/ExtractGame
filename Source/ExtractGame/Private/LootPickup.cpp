// Fill out your copyright notice in the Description page of Project Settings.


#include "LootPickup.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ALootPickup::ALootPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

}

// Called when the game starts or when spawned
void ALootPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALootPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

