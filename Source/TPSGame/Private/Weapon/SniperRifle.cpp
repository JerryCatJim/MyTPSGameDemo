// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SniperRifle.h"
#include "Blueprint/UserWidget.h"
#include "SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ASniperRifle::ASniperRifle()
{
	ZoomedFOV = 10.f;
	ZoomInterpSpeed = 10.f;
	BulletSpread = 2;
	
	//从C++中获取蓝图类
	const FString WidgetClassLoadPath = FString(TEXT("'/Game/UI/WeaponScope/BP_SniperScopeUI.BP_SniperScopeUI_C'"));//蓝图一定要加_C这个后缀名
	ScopeWidgetClass = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
}

void ASniperRifle::BeginPlay()
{
	Super::BeginPlay();
	
	if(MyOwner)
	{
		MyOwnerController = Cast<ATPSPlayerController>(MyOwner->GetController());
		if(ScopeWidgetClass && MyOwner->IsLocallyControlled())
		{
			SniperScopeWidget = CreateWidget(MyOwnerController, ScopeWidgetClass);
			if(SniperScopeWidget)
			{
				SniperScopeWidget->AddToViewport();
				SniperScopeWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

float ASniperRifle::GetDynamicBulletSpread()
{
	if(CheckOwnerValidAndAlive())
	{
		float TempRate = 1;
		if(MyOwner->GetVelocity().Size2D() > 0)
		{
			TempRate *= MyOwner->GetIsAiming() ? 200 : 3;
		}
		if(MyOwner->GetIsAiming())
		{
			TempRate *= 0.01;
		}
		if(MyOwner->GetCharacterMovement() && MyOwner->GetCharacterMovement()->IsFalling())
		{
			TempRate *= MyOwner->GetIsAiming() ? 300 : 3;
		}
		TempRate = FMath::Clamp(TempRate, 0.01f, 8.0f);
		return BulletSpread * TempRate;
	}
	return BulletSpread;
}

void ASniperRifle::PostReload()
{
	Super::PostReload();
	
	ResetWeaponZoom();
}

void ASniperRifle::DealWeaponZoom()
{
	Super::DealWeaponZoom();
	ShowSniperScope(true);
}

void ASniperRifle::DealWeaponResetZoom()
{
	Super::DealWeaponResetZoom();
	ShowSniperScope(false);
}

void ASniperRifle::DealPlayTraceEffect(FVector TraceEnd)
{
	//本地在开镜时不显示弹道轨迹(烟雾从左侧出来，不好看)
	if(MyOwner && MyOwner->IsLocallyControlled() && MyOwner->GetIsAiming())
	{
		return;
	}
	
	Super::DealPlayTraceEffect(TraceEnd);
}

void ASniperRifle::ShowSniperScope_Implementation(bool ShowScopeView)
{
	if(!ScopeWidgetClass || !SniperScopeWidget)
	{
		return;
	}

	if(ShowScopeView)
	{
		SniperScopeWidget->SetVisibility(ESlateVisibility::Visible);
		if(ZoomInSound)
		{
			const FVector CameraLocation = MyOwner->GetCameraComponent()->GetComponentLocation();
			UGameplayStatics::PlaySoundAtLocation(this, ZoomInSound, CameraLocation + CameraLocation.ForwardVector * 10 );
			if(MyOwnerController)
			{
				MyOwnerController->SetCrossHairVisibility(false);
			}
		}
	}
	else
	{
		SniperScopeWidget->SetVisibility(ESlateVisibility::Collapsed);
		if(ZoomOutSound)
		{
			const FVector CameraLocation = MyOwner->GetCameraComponent()->GetComponentLocation();
			UGameplayStatics::PlaySoundAtLocation(this, ZoomOutSound, CameraLocation + CameraLocation.ForwardVector * 10 );
			if(MyOwnerController)
			{
				MyOwnerController->SetCrossHairVisibility(true);
			}
		}
	}
}

void ASniperRifle::Destroyed()
{
	Super::Destroyed();
	
	if(SniperScopeWidget)
	{
		SniperScopeWidget->RemoveFromParent();
	}
}
