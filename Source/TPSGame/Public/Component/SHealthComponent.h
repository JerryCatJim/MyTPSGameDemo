// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHealthComponent.generated.h"

//自定义委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, class USHealthComponent*, OwningHealthComponent, float, Health, float, HealthDelta, //HealthDelta 生命值改变量,增加或减少
	const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

DECLARE_DELEGATE(FTestDelegate)

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSGAME_API USHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USHealthComponent();

	// Called every frame
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//生命值恢复函数
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = HealthComponent)
	void Heal(float HealAmount, class AController* InstigatedBy = nullptr, AActor* Healer = nullptr);
	
	//伤害处理函数
	UFUNCTION(BlueprintCallable, Server, Reliable, Category= TakeDamage)
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//双端发送广播函数，为了能传递多个参数而没有用OnRep_Health()
	UFUNCTION(NetMulticast, Reliable)
	void Multi_OnHealthChangedBoardCast(float CurHealth, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* Causer);
	
	//每当Health变量被网络同步复制一次，就由服务器通知，在客户端触发一次OnRep_Health函数
	UFUNCTION()  //必须用UFUNCTION标记
	void OnRep_Health(float OldHealth);

public:
	//生命值变更时触发事件广播
	UPROPERTY(BlueprintAssignable, Category= Events)
	FOnHealthChangedSignature OnHealthChanged;

	FTestDelegate TestDelegate;
protected:
	//生命值
	UPROPERTY(ReplicatedUsing= OnRep_Health, EditAnywhere, BlueprintReadOnly, Category= HealthComponent)
	float Health;
	//默认生命值
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= HealthComponent)
	float MaxHealth;
	//无限生命
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= HealthComponent)
	bool bIsInfinityHealth = false;
	//无敌
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= HealthComponent)
	bool bIsInvincible = false;

	UPROPERTY()
	APawn* MyOwnerPawn;
};
