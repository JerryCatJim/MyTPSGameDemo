// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SBuffComponent.generated.h"

class ASPowerUpActor;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSGAME_API USBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USBuffComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddToBuffList(int PowerUpID, ASPowerUpActor* PowerUpActor, AActor* OverlapActor);
	
protected:
	//Buff列表
	UPROPERTY()  //加UPROPERTY 防止垃圾回收
	TMap<int, ASPowerUpActor*> HashMap;
public:
		
};
