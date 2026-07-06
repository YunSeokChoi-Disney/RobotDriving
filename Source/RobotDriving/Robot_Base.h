// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Robot_Base.generated.h"

// 델리게이트 이름이 다른 헤더와 겹치지 않도록 Robot을 붙여 변경.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRobotMessageReceived, int32, RobotClientID, const FString&, Message);

UCLASS()
class ROBOTDRIVING_API ARobot_Base : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARobot_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 서버와 연결된 소켓
	FSocket* ClientSocket;

	// 주기적으로 서버로부터 데이터를 받기 위한 타이머
	FTimerHandle ReceiveTimerHandle;
	void CheckForIncomingData();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 서버에 접속을 시도하는 함수 (블루프린트에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Robot Network")
	bool ConnectToServer(const FString& IPAddress, int32 Port);

	// 서버로 문자열을 보내는 함수
	UFUNCTION(BlueprintCallable, Category = "Robot Network")
	void SendMessageToServer(const FString& Message);

	// 변경된 델리게이트 타입을 적용했습니다.
	UPROPERTY(BlueprintAssignable, Category = "Robot Network")
	FOnRobotMessageReceived OnMessageReceived;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};