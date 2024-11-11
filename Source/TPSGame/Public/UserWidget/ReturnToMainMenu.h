// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void MenuSetup();
	void MenuTearDown();

	void ShowOrHideReturnToMainMenu();
	
protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnPlayerLeftGame();
	
private:
	UFUNCTION()
	void ReturnButtonClicked();

public:
	UPROPERTY(EditDefaultsOnly)
	bool bShowWidgetAfterCreate = false;

	UPROPERTY(EditDefaultsOnly)
	bool bCanAddToViewport = true;  //已经有parentview的就不要加入viewport
	
private:
	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnButton;

	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionSubsystem;

	UPROPERTY()
	class ATPSPlayerController* MyPlayerController;
	
	bool bShowWidget = false;
};
