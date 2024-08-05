// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Component/RocketMovementComponent.h"

UProjectileMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(
	const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	//return Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	//Super::HandleImpact(Hit, TimeSlice, MoveDelta);
	//Do Nothing. Rockets should not stop; only explode when their CollisionBox detects a hit.
}
