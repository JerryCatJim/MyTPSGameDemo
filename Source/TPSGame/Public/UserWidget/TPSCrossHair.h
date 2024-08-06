// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TPSCrossHair.generated.h"

class UImage;
/**
 * 
 */
UENUM(BlueprintType)
enum ECrossHairPos
{
	Left,
	Right,
	Up,
	Down,
	Center
};

UCLASS()
class TPSGAME_API UTPSCrossHair : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override; 
	
	UFUNCTION(BlueprintCallable)
	void RebindMyOwner();

	UImage* GetValidCrossHairLine(bool& OutIsLeftOrRight);
	float GetCrossHairPos(UImage* CrossHair);
	
	void DrawCrossHairSpread();

	float GetDynamicSpreadValue();

	float GetSpreadSpeed();

	bool CheckIsCrossHairHasSpreadOrShrunk(UImage* CrossHair, bool IsLeftOrRight);

	void SetCrossHairLineNewPos(UImage* CrossHair, ECrossHairPos CrossHairPos );
	
	void SetCrossHairPos();

	void FixAllCrossHairPos();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	float DefaultPosValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	float SpreadUnitQuantity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	float ZeroToMaxSpreadTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	float MaxSpreadRate = 10.f;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair, meta = (BindWidget))
	UImage* C_Left;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair, meta = (BindWidget))
	UImage* C_Right;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair, meta = (BindWidget))
	UImage* C_Up;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair, meta = (BindWidget))
	UImage* C_Down;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair, meta = (BindWidget))
	UImage* C_Center;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CrossHair)
	class ASCharacter* MyOwner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	float LastSpreadValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	FTimerHandle FSpreadCrossHairTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CrossHair)
	bool IsSpreading = false;
};
