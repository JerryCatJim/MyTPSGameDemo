#include "MySocketServerSubsystem.h"

#include <string>

#include "INetworkingWebSocket.h"
#include "IWebSocketNetworkingModule.h"
#include "LogMySocketServer.h"
#include "WebSocketNetworkingDelegates.h"

#pragma region InitSubsystem
bool UMySocketServerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UMySocketServerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMySocketServerSubsystem::Deinitialize()
{
	StopMyWebSocketServer();
	Super::Deinitialize();
}

void UMySocketServerSubsystem::Tick(float DeltaTime)
{
	if(IsWebSocketRunning())
	{
		MyWebSocketServer->Tick();
	}
}

bool UMySocketServerSubsystem::IsTickable() const
{
	return !IsTemplate();
}

TStatId UMySocketServerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMySocketServerSubsystem, STATGROUP_Tickables); 
}
#pragma endregion InitSubsystem

bool UMySocketServerSubsystem::StartMyWebSocketServer(int32 ServerPort)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s"), *FString(__FUNCTION__));

	//客户端链接创建的回调，不是子系统自己创建的回调
	FWebSocketClientConnectedCallBack CallBack;
	CallBack.BindUObject(this, &UMySocketServerSubsystem::OnWebSocketClientConnected);

	if(MyWebSocketServer && MyWebSocketServer.IsValid())  //单例，已经存在不创建
	{
		UE_LOG(LogMySocketServer, Warning, TEXT("%s : WebSocketServer Is Running"), *FString(__FUNCTION__));
		return false;
	}
	
	MyWebSocketServer = FModuleManager::Get().LoadModuleChecked<IWebSocketNetworkingModule>(TEXT("WebSocketNetWorking")).CreateServer();

	if(!MyWebSocketServer || !MyWebSocketServer->Init(ServerPort, CallBack))  //创建或初始化失败
	{
		UE_LOG(LogMySocketServer, Warning, TEXT("%s : WebSocketServer Init Failed"), *FString(__FUNCTION__));
		MyWebSocketServer.Reset();
		return false;
	}

	UE_LOG(LogMySocketServer, Display, TEXT("%s : WebSocketServer Init Succeed"), *FString(__FUNCTION__));
	return true;
}

void UMySocketServerSubsystem::StopMyWebSocketServer()
{
	UE_LOG(LogMySocketServer, Display, TEXT("%s"), *FString(__FUNCTION__));
	if(IsWebSocketRunning())
	{
		MyWebSocketServer.Reset();
	}
}

void UMySocketServerSubsystem::SendMessageToAllClients(FString Message)
{
	FTCHARToUTF8 UTF8Str(*Message);
	const int32 UTF8StrLen = UTF8Str.Length();
	TArray<uint8> UTF8Array;
	UTF8Array.SetNum(UTF8StrLen);
	memcpy(UTF8Array.GetData(), UTF8Str.Get(), UTF8StrLen);
	/*for(uint8& chr : UTF8Str)
	{
		UTF8Array.Add(chr);
	}*/
	for(FMyWebSocketConnection& SocketConnection : MyWebSocketConnections)
	{
		SocketConnection.WebSocket->Send(UTF8Array.GetData(), UTF8Array.Num(), false);
	}
}

void UMySocketServerSubsystem::SendMessageToPointedClient(FGuid InTargetClientId, FString Message)
{
	FTCHARToUTF8 UTF8Str(*Message);
	const int32 UTF8StrLen = UTF8Str.Length();
	TArray<uint8> UTF8Array;
	UTF8Array.SetNum(UTF8StrLen);
	memcpy(UTF8Array.GetData(), UTF8Str.Get(), UTF8StrLen);
	/*for(uint8& chr : UTF8Str)
	{
		UTF8Array.Add(chr);
	}*/
	if(FMyWebSocketConnection* SocketConnection = MyWebSocketConnections.FindByPredicate(
		[&InTargetClientId](const FMyWebSocketConnection& MyConnection)->bool{ return MyConnection.Id == InTargetClientId; }))
	{
		SocketConnection->WebSocket->Send(UTF8Array.GetData(), UTF8Array.Num(), false);
	}
}

bool UMySocketServerSubsystem::CheckWebSocketConnectIsValid(FGuid InTargetClientId)
{
	if(IsWebSocketRunning())
	{
		int32 Index = MyWebSocketConnections.IndexOfByPredicate([InTargetClientId](const FMyWebSocketConnection& Connection)->bool{ return Connection.Id == InTargetClientId; });
		if(Index != INDEX_NONE)
		{
			return true;
		}
	}
	return false;
}

void UMySocketServerSubsystem::OnWebSocketClientConnected(INetworkingWebSocket* InWebSocket)
{
	UE_LOG(LogMySocketServer, Display, TEXT("%s"), *FString(__FUNCTION__));
	if(InWebSocket)
	{
		FMyWebSocketConnection MyWebSocketConnection = FMyWebSocketConnection(InWebSocket);
		
		FWebSocketInfoCallBack ConnectedCallBack;
		ConnectedCallBack.BindUObject(this, &UMySocketServerSubsystem::OnConnectedCallBack, MyWebSocketConnection.Id);
		InWebSocket->SetConnectedCallBack(ConnectedCallBack);
		
		FWebSocketInfoCallBack ErrorCallBack;
		ErrorCallBack.BindUObject(this, &UMySocketServerSubsystem::OnErrorCallBack, MyWebSocketConnection.Id);
		InWebSocket->SetErrorCallBack(ErrorCallBack);
		
		FWebSocketPacketReceivedCallBack ReceiveCallBack;
		//ReceiveCallBack.BindUObject(this, &UMySocketServerSubsystem::OnReceiveCallBack);
		InWebSocket->SetReceiveCallBack(ReceiveCallBack);
		
		FWebSocketInfoCallBack ClosedCallBack;
		ClosedCallBack.BindUObject(this, &UMySocketServerSubsystem::OnClosedCallBack, MyWebSocketConnection.Id);
		InWebSocket->SetSocketClosedCallBack(ClosedCallBack);

		MyWebSocketConnections.Add(MoveTemp(MyWebSocketConnection));
	}
	
	
}

bool UMySocketServerSubsystem::IsWebSocketRunning()
{
	return MyWebSocketServer && MyWebSocketServer.IsValid();
}

void UMySocketServerSubsystem::OnConnectedCallBack(FGuid ClientId)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s--ClientID : %s"), *FString(__FUNCTION__), *ClientId.ToString());
}

void UMySocketServerSubsystem::OnErrorCallBack(FGuid ClientId)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s--ClientID : %s"), *FString(__FUNCTION__), *ClientId.ToString());
}

void UMySocketServerSubsystem::OnReceiveCallBack(void* Data, int32 DataSize, FGuid ClientId)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s--ClientID : %s"), *FString(__FUNCTION__), *ClientId.ToString());

	TArrayView<uint8> DataArrayView = MakeArrayView(static_cast<uint8*>(Data), DataSize);
	const std::string cstr(reinterpret_cast<const char*>(
		DataArrayView.GetData()),
		DataArrayView.Num());
	FString StrInfo = UTF8_TO_TCHAR(cstr.c_str());

	//TODO: 没有实现心跳机制,接收的不是心跳信息时才应该走下面分发信息
	ProcessAllClientInformation(ClientId, StrInfo);
}

void UMySocketServerSubsystem::OnClosedCallBack(FGuid ClientId)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s--ClientID : %s"), *FString(__FUNCTION__), *ClientId.ToString());
	int32 Index = MyWebSocketConnections.IndexOfByPredicate([ClientId](const FMyWebSocketConnection& Connection)->bool{ return Connection.Id == ClientId; });
	if(Index!=INDEX_NONE)
	{
		//发出通知，该链接已关闭
		MyWebSocketClosedCallBack.ExecuteIfBound(ClientId);
		MyWebSocketConnections.RemoveAtSwap(Index);
	}
}

void UMySocketServerSubsystem::ProcessAllClientInformation(FGuid ClientId, FString Information)
{
	MyWebSocketReceiveCallBack.ExecuteIfBound(ClientId, Information);
}
