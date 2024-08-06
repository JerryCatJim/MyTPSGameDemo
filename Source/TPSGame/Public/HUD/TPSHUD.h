// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPSHUD.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ResetCrossHairWidget(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable)
	void RemoveCrossHairWidget();
	
	UFUNCTION(BlueprintCallable)
	void AddToWidgetList(UUserWidget* WidgetToRecord);

	virtual void Destroyed() override;
	
protected:
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<UUserWidget*> WidgetsList;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UTPSCrossHair* CrossHairView;
};
