#include "Robot_Base.h"


ARobot_Base::ARobot_Base()
{
	PrimaryActorTick.bCanEverTick = true;
	ClientSocket = nullptr;
}

void ARobot_Base::BeginPlay()
{
	Super::BeginPlay();
}

bool ARobot_Base::ConnectToServer(const FString& IPAddress, int32 Port)
{
	// 1. 소켓 서브시스템으로부터 TCP 소켓 생성
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem) return false;

	ClientSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("RobotClientSocket"), false);
	if (!ClientSocket) return false;

	// 2. 원격 서버 주소 파싱
	FIPv4Address TargetAddress;
	if (!FIPv4Address::Parse(IPAddress, TargetAddress))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid IP Address Format: %s"), *IPAddress);
		return false;
	}

	TSharedRef<FInternetAddr> RemoteAddr = SocketSubsystem->CreateInternetAddr();
	RemoteAddr->SetIp(TargetAddress.Value);
	RemoteAddr->SetPort(Port);

	// 3. 서버에 접속 시도
	bool bConnected = ClientSocket->Connect(*RemoteAddr);

	if (bConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully Connected to Server %s:%d"), *IPAddress, Port);

		// 데이터 수신을 위한 타이머 시작 (0.01초 주기)
		GetWorldTimerManager().SetTimer(ReceiveTimerHandle, this, &ARobot_Base::CheckForIncomingData, 0.01f, true);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to Connect to Server"));
		SocketSubsystem->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
		return false;
	}
}

void ARobot_Base::CheckForIncomingData()
{
	if (!ClientSocket) return;

	uint32 Size;
	// 서버가 보낸 데이터가 있는지 확인
	if (ClientSocket->HasPendingData(Size))
	{
		TArray<uint8> ReceivedData;
		ReceivedData.SetNumUninitialized(Size);

		int32 Read = 0;
		if (ClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read))
		{
			FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
			ReceivedString.LeftInline(Read);

			// 메인 게임 스레드에서 안전하게 로직 처리 (델리게이트 브로드캐스트나 폰 제어 등)
			AsyncTask(ENamedThreads::GameThread, [this, ReceivedString]()
				{
					UE_LOG(LogTemp, Warning, TEXT("Received From Server: %s"), *ReceivedString);

					// 여기에 서버가 보낸 명령(예: 이동 명령, 좌표 등)을 해석하는 파싱 로직을 넣으면 됩니다.
					// 가령 "MOVE_TO_X_Y" 같은 문자열을 파싱해서 AddMovementInput으로 캐릭터를 움직일 수 있죠.
				});
		}
	}

	// 서버와의 연결이 끊겼는지 체크
	if (ClientSocket->GetConnectionState() == SCS_ConnectionError)
	{
		UE_LOG(LogTemp, Error, TEXT("Server connection lost."));
		EndPlay(EEndPlayReason::RemovedFromWorld);
	}
}

void ARobot_Base::SendMessageToServer(const FString& Message)
{
	if (!ClientSocket || ClientSocket->GetConnectionState() != SCS_Connected) return;

	FTCHARToUTF8 Converter(*Message);
	int32 Sent = 0;
	ClientSocket->Send((uint8*)Converter.Get(), Converter.Length(), Sent);
}

void ARobot_Base::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearTimer(ReceiveTimerHandle);

	if (ClientSocket)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Client Socket Closed."));
	}
}

void ARobot_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 예시: 주기적으로 캐릭터의 현재 위치를 서버로 전송하고 싶다면?
	// Static 타이머나 Tick 카운터를 활용해 SendMessageToServer로 현재 좌표를 쏴줄 수 있습니다.
}

void ARobot_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}