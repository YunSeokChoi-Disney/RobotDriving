// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sockets.h"            // Core socket class definitions (FSocket)
#include "SocketSubsystem.h"    // Cross-platform socket management
#include "Common/TcpSocketBuilder.h" // Streamlined TCP socket construction
#include "Common/TcpListener.h"
#include "Interfaces/IPv4/IPv4Address.h" // Handles target IP formatting

#include "RobotStationBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRobotConnected, int32, RobotClientID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMessageReceived, int32, RobotClientID, const FString&, Message);




UCLASS()
class ROBOTDRIVING_API ARobotStationBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARobotStationBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TSharedPtr<FTcpListener> TcpListener;
	FSocket* ListenSocket;


	TMap<int32, FSocket*> ConnectedRobots;
	int32 NextClientID;

	//클라이언트 접속 처리 함수
	bool OnConnectionAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndPoint);

	//주기적으로 데이터를 읽어오기 위한 타이머
	void CheckForIncomingData();
	FTimerHandle DataCheckTimerHandle;


	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Robot Network")
	bool StartServer(int32 Port);

	//특정 로봇에게 메시지 전송하는 함수
	UFUNCTION(BlueprintCallable, Category="Robot Network")
	void SendMessageToRobot(int32 RobotClientID, const FString& Message);


	//블루프린트에서 바인딩할 이벤트

	UPROPERTY(BlueprintAssignable, Category="Robot Network")
	FOnRobotConnected OnRobotConnected;

	UPROPERTY(BlueprintAssignable, Category = "Robot Network")
	FOnMessageReceived OnMessageReceived;

	


};
