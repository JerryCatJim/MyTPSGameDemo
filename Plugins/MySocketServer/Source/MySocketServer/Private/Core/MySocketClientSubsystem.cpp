#include "MySocketClientSubsystem.h"

#include "IWebSocket.h"
#include "WebSocketsModule.h"
#include "LogMySocketServer.h"


#pragma region InitSubsystem
bool UMySocketClientSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UMySocketClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

}

void UMySocketClientSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UMySocketClientSubsystem::Tick(float DeltaTime)
{

}

bool UMySocketClientSubsystem::IsTickable() const
{
	return !IsTemplate();
}

TStatId UMySocketClientSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UXGHelperGameInstanceSubsystem, STATGROUP_Tickables);
}

#pragma endregion InitSubsystem

void UMySocketClientSubsystem::CreateSocket()
{
	//FModuleManger::Get().LoadModuleChecked("WebSockets");

	FString ServerURL = TEXT("ws://127.0.0.1:9093");  //127.0.0.1意思为本机IP
	FString ServerProtocol = TEXT("");

	Socket = FWebSocketsModule::Get().CreateWebSocket(ServerURL, ServerProtocol);
	Socket->OnConnected().AddUObject(this, &UMySocketClientSubsystem::OnConnected);
	Socket->OnConnectionError().AddUObject(this, &UMySocketClientSubsystem::OnConnectionError);
	Socket->OnClosed().AddUObject(this, &UMySocketClientSubsystem::OnClosed);
	Socket->OnMessage().AddUObject(this, &UMySocketClientSubsystem::OnMessage);
	Socket->OnMessageSent().AddUObject(this, &UMySocketClientSubsystem::OnMessageSent);

	Socket->Connect();
}

void UMySocketClientSubsystem::CloseSocket()
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s"), *FString(__FUNCTION__));
}

void UMySocketClientSubsystem::OnConnected()
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s"), *FString(__FUNCTION__));

	FString FirstMessage = TEXT("Hello I am a client.");
	Socket->Send(*FirstMessage);
}

void UMySocketClientSubsystem::OnConnectionError(const FString& Error)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s Error : %s"), *FString(__FUNCTION__), *Error);
}

void UMySocketClientSubsystem::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s CloseReason : %s"), *FString(__FUNCTION__), *Reason);
}

void UMySocketClientSubsystem::OnMessage(const FString& Message)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s"), *FString(__FUNCTION__));
}

void UMySocketClientSubsystem::OnMessageSent(const FString& MessageString)
{
	UE_LOG(LogMySocketServer, Warning, TEXT("%s"), *FString(__FUNCTION__));
}
