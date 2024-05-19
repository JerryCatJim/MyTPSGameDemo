#pragma once
#include "CoreMinimal.h"

#include "MySocketServerType.h"
#include "IWebSocketServer.h"
#include "MySocketServerSubsystem.generated.h"

#pragma region FMyWebSocketConnection Struct
USTRUCT()
struct FMyWebSocketConnection
{
	GENERATED_BODY()
public:
	explicit FMyWebSocketConnection(INetworkingWebSocket* InWebSocket)
		: WebSocket(InWebSocket)
		,Id(FGuid::NewGuid())
	{
		
	}

	FMyWebSocketConnection(){}
	
	FMyWebSocketConnection(FMyWebSocketConnection&& InConnection) noexcept
		:Id(InConnection.Id)
	{
		WebSocket = InConnection.WebSocket;
		InConnection.WebSocket = nullptr;
	}

	~FMyWebSocketConnection()
	{
		if(WebSocket)
		{
			//delete WebSocket;
			WebSocket = nullptr;
		}
	}

	FMyWebSocketConnection& operator=(const FMyWebSocketConnection& InConnection)
	{
		WebSocket = InConnection.WebSocket;
		Id = InConnection.Id;
	}
	
	//FMyWebSocketConnection(const FMyWebSocketConnection&) = delete;
	//FMyWebSocketConnection& operator=(const FMyWebSocketConnection&) = delete;
	//FMyWebSocketConnection& operator=(FMyWebSocketConnection&&) = delete;
	
	INetworkingWebSocket* WebSocket = nullptr;
	FGuid Id;
};
#pragma endregion FMyWebSocketConnection Struct

UCLASS()
class UMySocketServerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

#pragma region InitSubsystem
	
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

#pragma endregion InitSubsystem
	
public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "StartWebSocketServer"), Category = "MyWebSocketServer")
	bool StartMyWebSocketServer(int32 ServerPort = 9093);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "StopWebSocketServer"), Category = "MyWebSocketServer")
	void StopMyWebSocketServer();

	//不应该暴露给蓝图，这里是为了测试
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SendMessageToAllClients"), Category = "MyWebSocketServer")
	void SendMessageToAllClients(FString Message);
	
	//UFUNCTION(BlueprintCallable, meta = (DisplayName = "SendMessageToPointedClient"), Category = "MyWebSocketServer")
	void SendMessageToPointedClient(FGuid InTargetClientId, FString Message);

	bool CheckWebSocketConnectIsValid(FGuid InTargetClientId);

	
protected:
	void OnWebSocketClientConnected(INetworkingWebSocket* InWebSocket);
	bool IsWebSocketRunning();

	void OnConnectedCallBack(FGuid ClientId);
	void OnErrorCallBack(FGuid ClientId);
	void OnReceiveCallBack(void* Data, int32 DataSize, FGuid ClientId);
	void OnClosedCallBack(FGuid ClientId);

	void ProcessAllClientInformation(FGuid ClientId, FString Information);
	
public:
	FMyWebSocketReceiveCallBack MyWebSocketReceiveCallBack;
	FMyWebSocketClosedCallBack MyWebSocketClosedCallBack;
	
private:
	TUniquePtr<IWebSocketServer> MyWebSocketServer;
	TArray<FMyWebSocketConnection> MyWebSocketConnections;
};
