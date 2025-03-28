// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseWeapon/SWeapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "PickUpWeapon.generated.h"

UCLASS()
class TPSGAME_API APickUpWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickUpWeapon();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ShowTipWidget(bool bIsVisible);

	UFUNCTION(BlueprintCallable)
	void ShowTipWidgetOnOwningClient();
	
	UFUNCTION(BlueprintCallable)
	void TryPickUpWeapon();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetCanBePickedUp(bool CanBePickedUp);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_SetCanBePickedUp(bool CanBePickedUp);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_WeaponPickUpInfo();

	UFUNCTION(BlueprintCallable, Server, Reliable)  //刷新PickUpWeapon的值为输入值
	void Server_ResetPickUpWeaponInfo(FWeaponPickUpInfo NewInfo);

	UFUNCTION()
	void OnCapsuleComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	void OnCapsuleComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnPlayerDead(AController* InstigatedBy, AActor* DamageCauser,const UDamageType* DamageType);

	UFUNCTION()
	void OnInteractKeyUp();
	UFUNCTION()
	void OnInteractKeyDown();
	UFUNCTION()
	void OnInteractKeyLongPressed();

	UFUNCTION(NetMulticast, Reliable)
	void ThrowAfterSpawn();

private:
	
public:
	UPROPERTY(EditAnywhere, Category=Component)
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Component)
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Component)
	UWidgetComponent* WidgetComponent;

	//蓝图中提前设置的要生成的武器类，动态生成的话不需要设定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TSubclassOf<ASWeapon> DefaultEditWeaponClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USkeletalMesh* PreviewWeaponMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_WeaponPickUpInfo)
	FWeaponPickUpInfo WeaponPickUpInfo;
	//最初被设定的提供的武器
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)//, Replicated)
	FWeaponPickUpInfo OriginalWeaponInfo;
	
	UPROPERTY(VisibleAnywhere)
	ASCharacter* LastOverlapPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)  //武器是否可以掉落在地面(拖进场景时默认悬浮，人物死亡后掉落时落到地上)
	bool bCanMeshDropOnTheGround = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bCanInteractKeyLongPress = true;  //写在Protected里然后Public用Getter Setter更好，这里懒了就不做了

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bThrowAfterSpawn = false;
	
protected:
	
private:
	//同一时间只能有一个可拾取武器发生重叠时间，记录正在发生重叠的武器是不是此武器自己
	bool bIsThisTheOverlapOne = false;

	//防止客户端在未完成一次拾取流程之前多次按键快速拾取武器导致刷新数据错误，加个锁
	bool bCanBePickedUp = true;
};
