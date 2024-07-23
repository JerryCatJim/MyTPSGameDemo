// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Weapon/SWeapon.h"
#include "TPSCharacter_AnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API UTPSCharacter_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(BlueprintReadOnly, Category=Character, meta = (AllowPrivateAccess = "true"))
	class ASCharacter* PlayerCharacter;
	
	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;
	
	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	float DirectionDegree;

	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	float AimOffset_Y;

	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	float AimOffset_Z;

	UPROPERTY(BlueprintReadOnly, Category=Weapon, meta = (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;

	UPROPERTY(BlueprintReadOnly, Category=Weapon, meta = (AllowPrivateAccess = "true"))
	class ASWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category=Weapon, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<EWeaponType> EquippedWeaponType;

	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransToHoldWeapon;

	UPROPERTY(BlueprintReadOnly, Category=Movement, meta = (AllowPrivateAccess = "true"))
	FTransform RightHandTransToMoveToLeftHand;
};
