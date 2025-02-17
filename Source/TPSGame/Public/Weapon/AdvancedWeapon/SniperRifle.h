// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BaseWeapon/HitScanWeapon.h"
#include "SniperRifle.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ASniperRifle : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	ASniperRifle();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void Destroyed() override;

	virtual float GetDynamicBulletSpread() override;
	
	virtual void PreDealWeaponZoom() override;
	virtual void PreDealWeaponResetZoom() override;

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void ShowSniperScope(bool ShowScopeView);

	virtual void DealPlayTraceEffect(FVector TraceEnd) override;

	virtual void PostReload() override;
	
private:
	UPROPERTY()
	UUserWidget* SniperScopeWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> ScopeWidgetClass;

	UPROPERTY(EditAnywhere, Category=WeaponSound)
	USoundCue* ZoomInSound;
	UPROPERTY(EditAnywhere, Category=WeaponSound)
	USoundCue* ZoomOutSound;

	UPROPERTY()
	class ATPSPlayerController* MyOwnerController;
};
