// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "StashSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class EXTRACTGAME_API UStashSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	//战利品清单表
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Stash")
	TMap<FName, int32> StashCnt;
};
