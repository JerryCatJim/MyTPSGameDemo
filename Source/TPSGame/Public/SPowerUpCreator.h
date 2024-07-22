// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerUpCreator.generated.h"

class USphereComponent;
class UDecalComponent;
class ASPowerUpActor;

//这是道具刷新点

UCLASS()
class TPSGAME_API ASPowerUpCreator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPowerUpCreator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//重新生成道具
	UFUNCTION()
	void Respawn();
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;  //重写重叠事件
	
protected:
	//可拾取的球形范围
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Component)
	USphereComponent* SphereComponent;

	// Decal 贴花
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Component)
	UDecalComponent* DecalComponent;

	//PickUp类
	UPROPERTY(EditAnywhere, Category = PickUps)
	TSubclassOf<ASPowerUpActor> PowerUpClass;

	//道具实例
	UPROPERTY()
	ASPowerUpActor* PowerUpInstance;

	//计时器句柄
	FTimerHandle TimerHandle_Respawn;

	//重新生成的时间间隔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= PickUps)
	float RespawnInterval;
};
