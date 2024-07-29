// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/RocketProjectile.h"

#include "Kismet/GameplayStatics.h"

ARocketProjectile::ARocketProjectile()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	const APawn* FiringPawn = GetInstigator();
	if(FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if(FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				MinDamage,
				GetActorLocation(),  //火箭弹命中时的圆心，以此向外衰减伤害
				InnerRadius,
				OuterRadius,
				1.f,
				UDamageType::StaticClass(),
				TArray<AActor*>(),  //不无视发射者自己，意味着可以炸到自己
				this,
				FiringController
				);
		}
	}

	//在命中前计算出伤害衰减
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
