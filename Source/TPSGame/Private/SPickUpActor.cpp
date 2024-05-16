// Fill out your copyright notice in the Description page of Project Settings.


#include "SPickUpActor.h"

#include "SCharacter.h"
#include "SPowerUpActor.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "../TPSGame.h"   //或者TPSGame/TPSGame.h


// Sets default values
ASPickUpActor::ASPickUpActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetSphereRadius(75.f);

	//SphereComponent->SetupAttachment(RootComponent);
	SetRootComponent(SphereComponent);
	//RootComponent = SphereComponent;
	
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(64,75,75);
	DecalComponent->SetRelativeRotation(FRotator(0,90,0));
	DecalComponent->SetRelativeLocation(FVector(0,0,0));
	
	//防止阻挡武器射线检测
	SphereComponent->SetCollisionResponseToChannel(Collision_Weapon, ECR_Ignore);
	
	RespawnInterval = 0;

	SetReplicates(true);  //默认打开网络复制
}

// Called when the game starts or when spawned
void ASPickUpActor::BeginPlay()
{
	Super::BeginPlay();
	
	if(HasAuthority())  //是否在服务端主控  旧版写法 GetOwnerRole() == ROLE_Authority 也可用
	{
		//生成道具
		Respawn();
	}
}

// Called every frame
void ASPickUpActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASPickUpActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	//角色与道具(此类)发生重叠时的逻辑
	if(HasAuthority() && PowerUpClass && PowerUpInstance)  //仅当服务器主控时运行以下逻辑
	{
		//将道具效果加入BuffComponent管理
		ASCharacter* CurrentPlayer = Cast<ASCharacter>(OtherActor);
		if(!CurrentPlayer)  //转化失败则无事发生
		{
			return;
		}
		
		if(IsValid(CurrentPlayer->GetBuffComponent()))
		{
			CurrentPlayer->GetBuffComponent()->AddToBuffList(PowerUpInstance->GetPowerUpID(), PowerUpInstance, OtherActor);
			PowerUpInstance = nullptr;
		}
		
		if(RespawnInterval > 0) //道具生成点重新生成道具
		{
			//设置定时器用于重新生成道具
			GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &ASPickUpActor::Respawn, RespawnInterval, false, RespawnInterval);
		}
	}
}

//重新生成道具
void ASPickUpActor::Respawn()
{
	if(PowerUpClass == nullptr)  //如果没指定生成何种道具则不生成
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerUpClass is nullptr in %s, need to update settings."), *GetName());
		return;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	PowerUpInstance = GetWorld()->SpawnActor<ASPowerUpActor>(PowerUpClass, GetTransform(), SpawnParameters);

	if(PowerUpInstance->GetPowerUpID() <= 0)  //如果道具ID不合法
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerUpID is Illegal in %s, need to update settings."), *GetName());
	}
}
