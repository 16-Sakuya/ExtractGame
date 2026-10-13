// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Exfil.generated.h"

//前置声明
class UBoxComponent;

UCLASS()
class EXTRACTGAME_API AExfil : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AExfil();
	bool isOverlap(const AActor* Actor) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* ExfilArea;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
