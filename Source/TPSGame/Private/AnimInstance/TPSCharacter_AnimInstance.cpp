// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/TPSCharacter_AnimInstance.h"

#include "SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UTPSCharacter_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<ASCharacter>(TryGetPawnOwner());
}

void UTPSCharacter_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<ASCharacter>(TryGetPawnOwner());
	}

	if(IsValid(PlayerCharacter))
	{
		Speed = PlayerCharacter->GetVelocity().Size2D();
		bIsInAir = PlayerCharacter->GetCharacterMovement()->IsFalling();
		DirectionDegree = CalculateDirection(PlayerCharacter->GetVelocity(), PlayerCharacter->GetActorRotation());

		AimOffset_Y = PlayerCharacter->GetAimOffset_Y();
		AimOffset_Z = PlayerCharacter->GetAimOffset_Z(); 
	}
}
