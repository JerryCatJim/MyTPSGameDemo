// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TPSGameType/SearchSessionResultType.h"
#include "TPSMenu.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum EMenuButtonType
{
	HostButton,
	JoinButton,
	FindButton,
};

UCLASS()
class TPSGAME_API UTPSMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPersonCPP/Maps/Lobby")));

protected:

	virtual bool Initialize() override;
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

	//
	// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	//
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnFindSessionResultsReceived(const TArray<struct FMySearchSessionResult>& SessionResults);
	
private:
	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	UFUNCTION()
	void FindButtonClicked();

	void MenuTearDown();

public:
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseSteam = false;

	UPROPERTY(BlueprintReadOnly)
	class UTPSGameInstance* MyGameInstance;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* JoinButton;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* FindButton;
	
private:
	UPROPERTY()
	TEnumAsByte<EMenuButtonType> LastClickedButton;

	// The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 NumPublicConnections{4};

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString MatchType{TEXT("FreeForAll")};

	FString PathToLobby{TEXT("")};
	FString FullPathWithOptions = "";
};
