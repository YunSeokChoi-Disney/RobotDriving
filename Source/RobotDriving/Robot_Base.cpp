// Fill out your copyright notice in the Description page of Project Settings.

#include "Robot_Base.h"

#include "Async/Async.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"

// Sets default values
ARobot_Base::ARobot_Base()
{
	PrimaryActorTick.bCanEverTick = true;
	ClientSocket = nullptr;
	MyClientID = -1;
}

// Called when the game starts or when spawned
void ARobot_Base::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARobot_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ARobot_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// 서버 종료 및 오브젝트 파괴 시 소켓 정리
void ARobot_Base::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ReceiveTimerHandle);

	if (ClientSocket)
	{
		ClientSocket->Close();

		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(ClientSocket);
		}
		ClientSocket = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// 서버 접속 함수 구현
bool ARobot_Base::ConnectToServer(const FString& IPAddress, int32 Port)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem) return false;

	FIPv4Address Address;
	if (!FIPv4Address::Parse(IPAddress, Address))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid IP Address: %s"), *IPAddress);
		return false;
	}

	TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
	TargetAddr->SetIp(Address.Value);
	TargetAddr->SetPort(Port);

	ClientSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("RobotClientSocket"), false);
	if (!ClientSocket) return false;

	// 버퍼 크기 설정
	int32 NewSize = 0;
	ClientSocket->SetReceiveBufferSize(1024 * 4, NewSize);
	ClientSocket->SetSendBufferSize(1024 * 4, NewSize);

	if (!ClientSocket->Connect(*TargetAddr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connect to server."));
		ClientSocket->Close();
		SocketSubsystem->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Successfully connected to server!"));

	GetWorldTimerManager().SetTimer(ReceiveTimerHandle, this, &ARobot_Base::CheckForIncomingData, 0.01f, true);

	return true;
}

// 데이터 수신 및 블루프린트 델리게이트 브로드캐스트
void ARobot_Base::CheckForIncomingData()
{
	if (!ClientSocket) return;

	uint32 Size;
	if (ClientSocket->HasPendingData(Size))
	{
		TArray<uint8> ReceivedData;
		ReceivedData.SetNumUninitialized(Size);

		int32 Read = 0;
		if (ClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read))
		{
			FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			ReceivedString.LeftInline(Read);

			// 메인 게임 스레드에서 안전하게 로직 처리 및 델리게이트 호출
			AsyncTask(ENamedThreads::GameThread, [this, ReceivedString]()
				{
					UE_LOG(LogTemp, Warning, TEXT("Received From Server: %s"), *ReceivedString);

					// 블루프린트로 데이터 패스 (기존 로직 유지)
					OnMessageReceived.Broadcast(MyClientID, ReceivedString);

					// 파이썬 서버로부터 특정 제어 명령이 수신되었는지 확인
					if (ReceivedString.Contains(TEXT("CMD_SET_EVADE_TRUE")))
					{
						// 블루프린트 이벤트 호출
						OnReceiveEvadeCommand();
					}
				});
		}
	}

	if (ClientSocket->GetConnectionState() == SCS_ConnectionError)
	{
		UE_LOG(LogTemp, Error, TEXT("Server connection lost."));
		EndPlay(EEndPlayReason::RemovedFromWorld);
	}
}

// 서버로 데이터 전송 함수 구현
void ARobot_Base::SendMessageToServer(const FString& Message)
{
	if (!ClientSocket) return;

	// 1. 뒤에 개행 문자(\n)가 붙은 새로운 문자열 생성
	FString SafeMessage = Message;
	if (!SafeMessage.EndsWith(TEXT("\n")))
	{
		SafeMessage += TEXT("\n");
	}

	// 2. TCHAR형 문자열을 UTF-8 바이트 배열로 인코딩
	FTCHARToUTF8 Converter(*SafeMessage);

	// 3. 변환된 결과물 포인터와 컨버터 내부에 기록된 실제 UTF-8 바이트 길이를 추출
	const uint8* DataPtr = reinterpret_cast<const uint8*>(Converter.Get());
	int32 TotalBytes = Converter.Length(); // 에러 유발하던 GetLength() 대신 표준 Length() 적용

	if (DataPtr && TotalBytes > 0)
	{
		int32 BytesSent = 0;
		// 4. 소켓을 통해 파이썬 서버로 바이트 데이터 전송
		ClientSocket->Send(DataPtr, TotalBytes, BytesSent);
	}
}