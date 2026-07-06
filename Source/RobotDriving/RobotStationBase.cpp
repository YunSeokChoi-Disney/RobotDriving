// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotStationBase.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

// Sets default values
ARobotStationBase::ARobotStationBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARobotStationBase::BeginPlay()
{
	Super::BeginPlay();

}

bool ARobotStationBase::StartServer(int32 Port)
{
	FIPv4Address Address;
	FIPv4Address::Parse(TEXT("0.0.0.0"), Address);

	FIPv4Endpoint Endpoint(Address, Port);

	//FTcp Listener 생성
	TcpListener = MakeShareable(new FTcpListener(Endpoint));    

	if (TcpListener.IsValid())
	{
		//클라이언트가 접속했을 때의 Callback 함수 바인딩
		TcpListener->OnConnectionAccepted().BindUObject(this, &ARobotStationBase::OnConnectionAccepted);


		//주기적으로 로봇들이 보낸 데이터가 있는지 확인
		GetWorldTimerManager().SetTimer(DataCheckTimerHandle, this, &ARobotStationBase::CheckForIncomingData, 0.01f, true);

		UE_LOG(LogTemp, Warning, TEXT("TCP Server Started on Port %d"), Port);
		return true;
	}
	return false;
}


bool ARobotStationBase::OnConnectionAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
	if (!ClientSocket)
		return false;

	int32 NewID = ++NextClientID;
	ConnectedRobots.Add(NewID, ClientSocket);

	//게임 스레드 밖에서 호출될 수 있으므로, 메인 스레드에서 안전하게 델리게이트를 실행
	//이 부분 좀 다시 짜야함



	UE_LOG(LogTemp, Warning, TEXT("Robot Client Connected! Assigned ID: %d"), NewID);
	return true;

}

void ARobotStationBase::CheckForIncomingData()
{
	TArray<int32> DisconnectedRobots;

	for (auto& Elem : ConnectedRobots)
	{
		int32 ClientID = Elem.Key;
		FSocket* Socket = Elem.Value;

		uint32 Size;

		//읽을 데이터가 있는지 확인

		if (Socket->HasPendingData(Size))
		{
			TArray<uint8> ReceivedData;
			ReceivedData.SetNumUninitialized(Size);

			int32 Read = 0;
			if (Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read))
			{

				//바이트 데이터를 FString으로 변환
				FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));

				//null 문자 처리
				ReceivedString.LeftInline(Read);

				OnMessageReceived.Broadcast(ClientID, ReceivedString);
			}
		}

		//소켓 연결 상태 체크

		if (Socket->GetConnectionState() == SCS_ConnectionError)
		{
			DisconnectedRobots.Add(ClientID);
		}
	}
	for (int32 DeadID : DisconnectedRobots)
	{
		FSocket* Socket = ConnectedRobots[DeadID];
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		ConnectedRobots.Remove(DeadID);
		UE_LOG(LogTemp, Warning, TEXT("Robot Client Disconnected: %d"), DeadID);

	}
}

void ARobotStationBase::SendMessageToRobot(int32 RobotClientID, const FString& Message)
{
	if (ConnectedRobots.Contains(RobotClientID))
	{
		FSocket* Socket = ConnectedRobots[RobotClientID];
		FTCHARToUTF8 Converter(*Message);
		int32 Sent = 0;
		Socket->Send((uint8*)Converter.Get(), Converter.Length(), Sent);
	}

}

void ARobotStationBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearTimer(DataCheckTimerHandle);

	if (TcpListener.IsValid())
	{
		TcpListener->Stop();

	}

	for (auto& Elem : ConnectedRobots)
	{
		Elem.Value->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Elem.Value);
	}
	ConnectedRobots.Empty();

}


// Called every frame
void ARobotStationBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

