// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Robot_Base.generated.h" // 툴이 꼬이지 않게 위쪽 전방선언을 지우고 깔끔하게 갑니다.

// 전방 선언은 사용할 포인터 타입만 명시합니다.
class FSocket;
class ISocketSubsystem;

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

	// 중계소 서버가 부여해 준 내 로봇 고유 ID
	UPROPERTY(BlueprintReadOnly, Category = "Robot Network")
	int32 MyClientID;

	/**
	 * ★ [추가] 파이썬 서버로부터 회피 제어 명령("CMD_SET_EVADE_TRUE")을 받았을 때
	 * 블루프린트 이벤트 그래프에서 직접 이벤트를 받아 블랙보드를 조작할 수 있도록 선언합니다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Robot Network")
	void OnReceiveEvadeCommand();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 서버에 접속을 시도하는 함수 (블루프린트에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Robot Network")
	bool ConnectToServer(const FString& IPAddress, int32 Port);

	// 서버로 문자열을 보내는 함수 (블루프린트에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Robot Network")
	void SendMessageToServer(const FString& Message);

	// 블루프린트에서 바인딩 가능한 델리게이트 변수
	UPROPERTY(BlueprintAssignable, Category = "Robot Network")
	FOnRobotMessageReceived OnMessageReceived;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};