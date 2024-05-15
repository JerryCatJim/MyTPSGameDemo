// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MyInterfaceTest.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UMyInterfaceTest : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TPSGAME_API IMyInterfaceTest
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = TestInterface)
	bool NotEvent_NativeTest();
	//若声明了BlueprintNativeEvent且想在C++中也实现该接口函数，需要再声明一个virtual(以保证可被覆盖)且同名(加Implemetation后缀)的函数
	virtual bool NotEvent_NativeTest_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = TestInterface)
	bool NotEvent_ImplementTest();

	//有返回值不能在蓝图中被当作事件，没有返回值则可以
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = TestInterface)
	void IsEvent_NativeTest();
	virtual void IsEvent_NativeTest_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = TestInterface)
	void IsEvent_ImplementTest();
};
