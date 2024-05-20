#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "MySocketClientSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class UMySocketClientSubsystem : public UEngineSubsystem,public FTickableGameObject
{
	GENERATED_BODY()


#pragma region InitSubsystem

public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection)override;
	virtual void Deinitialize()override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

#pragma endregion InitSubsystem

public:
	UFUNCTION(BlueprintCallable)
	void CreateSocket();
	UFUNCTION(BlueprintCallable)
	void CloseSocket();

protected:
	void OnConnected();
	void OnConnectionError(const FString& Error);
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void OnMessage(const FString& Message);
	void OnMessageSent(const FString& MessageString);
	
private:
	TSharedPtr<class IWebSocket> Socket;
};