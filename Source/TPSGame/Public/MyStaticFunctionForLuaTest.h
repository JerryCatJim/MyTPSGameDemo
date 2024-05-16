// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyStaticFunctionForLuaTest.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API UMyStaticFunctionForLuaTest : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	int GetIntTest();
};
