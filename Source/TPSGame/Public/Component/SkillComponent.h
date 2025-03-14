// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SkillComponent.generated.h"

class UUserWidget;
class UGameplayAbility;
class ASCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillPercentChanged, float, SkillPercent);
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSGAME_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	
	UFUNCTION(BlueprintCallable,BlueprintPure)
	float GetSkillChargePercent(){ return SkillChargePercent; }
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetSkillChargePercent(float NewSkillChargePercent);
	UFUNCTION(BlueprintCallable,BlueprintPure)
	bool GetIsUsingUltimateSkill(){ return bIsUsingUltimateSkill; }
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetIsUsingUltimateSkill(bool NewIsUsingUltimateSkill);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void AddSkillChargePercent(float AddPercent);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ActivateAbility(int AbilityIndex);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ActivateUltimateSkill();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_SkillChargePercent(){ OnSkillPercentChanged.Broadcast(SkillChargePercent); }

private:
	UFUNCTION()
	void LearnAllSkills(bool HasGameBegun);

	void SetAutoSkillChargeTimer(bool HasGameBegun);
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnSkillPercentChanged OnSkillPercentChanged;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=SkillComponent)
	TArray<TSubclassOf<UGameplayAbility>> SkillsAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=SkillComponent)
	TSubclassOf<UUserWidget> SkillPercentWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=SkillComponent)
	FGameplayTagContainer UltimateSkillTag;
	
	//需要设置其中的Tag数组元素
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=SkillComponent)
	TArray<FGameplayTagContainer> GameplayTagContainer;
	
protected:
	UPROPERTY()
	ASCharacter* MyOwnerPlayer;
	
	UPROPERTY(ReplicatedUsing = OnRep_SkillChargePercent, EditAnywhere, BlueprintReadOnly, Category=SkillComponent, meta=(ClampMin = 0.f, ClampMax = 100.f))
	float SkillChargePercent = 0;

	//是否正在施放终极技能，例如有些终极技能有持续时间
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated, Category=SkillComponent)
	bool bIsUsingUltimateSkill = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=SkillComponent)
	float AutoSkillChargePercent = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=SkillComponent)
	float AutoSkillChargeTime = 5.f;
	
private:
	FTimerHandle AutoSkillChargeTimer;
	
};
