// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TPSGameBPFLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API UTPSGameBPFLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Screen")
	static bool IsInScreenViewport(const UObject* WorldContextObject, const FVector& WorldPosition);
	 
};
