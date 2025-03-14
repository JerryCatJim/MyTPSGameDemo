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
	ATPSHUD();
	
	UFUNCTION(BlueprintCallable)
	void ResetCrossHairWidget(APlayerController* PlayerController);
	UFUNCTION(BlueprintCallable)
	void RemoveCrossHairWidget();

	UFUNCTION(BlueprintCallable)
	void ResetSkillPercentWidget(APlayerController* PlayerController);
	UFUNCTION(BlueprintCallable)
	void RemoveSkillPercentWidget();
	
	UFUNCTION(BlueprintCallable)
	void SetCrossHairVisibility(bool IsVisible);
	
	UFUNCTION(BlueprintCallable)
	void AddToWidgetList(UUserWidget* WidgetToRecord);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ShowEndGameScreen(int WinnerID, ETeam WinningTeam, int Score, bool IsTeamMode);
	
	virtual void Destroyed() override;
	
protected:
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<UUserWidget*> WidgetsList;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UTPSCrossHair* CrossHairView;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UUserWidget* HitFeedbackCrossHairView;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UUserWidget* SkillPercentView;
	
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UUserWidget> EndGameScreenClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UUserWidget* EndGameView;*/
	
private:
	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* BackgroundCanvasPanel;
};
