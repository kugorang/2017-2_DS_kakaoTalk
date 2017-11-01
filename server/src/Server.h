#pragma once

#include <Windows.h>
#include <map>
#include <list>
#include "Protocol.h"
#include "RingBuffer.h"
#include "SerializationBuffer.h"

#pragma comment (lib, "ws2_32.lib")

using namespace std;

#define SOCKETINFO_ARRAY_MAX 100
#define ROOM_MAX 20
#define ROOM_PEOPLE_MAX 20

// ȸ�� ���� ������ ���� ����ü
struct USERINFO
{
	int userNo;
	WCHAR id[ID_MAX_LEN];
	WCHAR pw[PW_MAX_LEN];
	WCHAR name[NAME_MAX_LEN];
	WCHAR phoneNum[PHONENUM_MAX_LEN];
};

// ���� ���� ������ ���� ����ü
struct SOCKETINFO
{
	USERINFO* userInfo;
	int enterRoomNo;
	bool alreadyRoom;

	RingBuffer recvQueue;
	RingBuffer sendQueue;
};

// �� ���� ������ ���� ����ü
struct ROOMINFO
{
	int roomNo;
	WCHAR* roomTitle;
	WORD roomTitleSize;
	BYTE numberOfPeople;

	list<SOCKETINFO*> sockInfo[ROOM_PEOPLE_MAX];
};

class Server
{
private:
	map<SOCKET, SOCKETINFO*> socketInfoMap;
	map<int, USERINFO*> userInfoMap;
	list<ROOMINFO*> roomInfoList;

	int totalSockets = 0;
	int totalRooms = 0;

	// --------------------------------------------------
	// ��Ʈ��ũ Select �Լ���
	// --------------------------------------------------
	// ���� �Լ� ���� ��� �� ����
	void ErrorQuit(WCHAR* msg);

	// FD_WRITE ó�� �Լ�
	int FDWriteProc(SOCKET sock, SOCKETINFO* socketInfo);

	// FD_READ ó�� �Լ�
	int FDReadProc(SOCKET sock, SOCKETINFO* socketInfo);

	// CheckSum�� ����� �Լ�
	int MakeCheckSum(WORD msgType, WORD payloadSize, SerializationBuffer* serializationBuffer);

	// Packet�� ó���ϴ� �Լ�
	void PacketProc(SOCKET sock, WORD type, SerializationBuffer* serializationBuffer);

	// 1. Request �α��� ó�� �Լ�
	void RecvRequestLogin(SOCKET sock, SerializationBuffer* serializationBuffer);

	// 2. Response �α��� ó�� �Լ�
	void SendResponseLogin(SOCKET sock, WCHAR* id, WCHAR *pw);

	// 3. Request ��ȭ�� ����Ʈ ó�� �Լ�
	void RecvRequestRoomList(SOCKET sock, SerializationBuffer* serializationBuffer);

	// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
	void SendResponseRoomList(SOCKET sock);

	// 5. Request ��ȭ�� ���� ó�� �Լ�
	void RecvRequestRoomCreate(SOCKET sock, SerializationBuffer* serializationBuffer);

	// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
	void SendResponseRoomCreate(SOCKET sock, WCHAR* roomTitle, WORD roomTitleSize);

	// 7. Request ��ȭ�� ���� ó�� �Լ�
	void RecvRequestRoomEnter(SOCKET sock, SerializationBuffer* serializationBuffer);

	// 8. Response ��ȭ�� ���� ó�� �Լ�
	void SendResponseRoomEnter(SOCKET sock, BYTE response, ROOMINFO* roomInfoPtr);

	// 9. Request ä�� �۽� ó�� �Լ�
	void RecvRequestChat(SOCKET sock, SerializationBuffer* serializationBuffer);

	// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
	void SendResponseChat(SOCKET sock, WORD msgSize, WCHAR* msg);

	// 11. Request �� ���� ó�� �Լ�
	void RecvRequestRoomLeave(SOCKET sock, SerializationBuffer* serializationBuffer);

	// 12. Response �� ���� (����) ó�� �Լ�
	void SendResponseRoomLeave(SOCKET sock);

	// 13. Response �� ���� (����) ó�� �Լ�
	void SendResponseRoomDelete(SOCKET sock, int roomNo);

	// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
	void SendResponseUserEnter(SOCKET sock, SOCKETINFO* socketInfo);

	// 15. Request ȸ�� ���� ó�� �Լ�
	void RecvRequestJoin(SOCKET sock, SerializationBuffer *serializationBuffer);

	// 16. Response ȸ�� ���� ó�� �Լ�
	void SendResponseJoin(SOCKET sock, WCHAR *id, WCHAR *pw, WCHAR *name, WCHAR *phoneNum);

	// 17. Request ȸ�� ���� ���� �Լ�
	void RecvRequestEditInfo(SOCKET sock, SerializationBuffer *SerializationBuffer);

	// 18. Response ȸ�� ���� ���� �Լ�
	void SendResponseEditInfo(SOCKET sock, WCHAR *id, WCHAR *pw, WCHAR *name, WCHAR *phoneNum);

	// ���� ���� �Լ�
	BOOL AddSocketInfo(SOCKET sock);
	map<SOCKET, SOCKETINFO*>::iterator RemoveSocketInfo(SOCKET sock);

	// ���� ã�� �Լ�
	ROOMINFO* FindRoom(int roomNo);

	// �� ������Ը� �۽��ϴ� �Լ�
	void SendUnicast(SOCKET sock, NetworkPacketHeader networkPacketHeader,
		SerializationBuffer* serializationBuffer);

	// �� ������Ը� �۽��ϴ� �Լ�
	void SendUnicast(SOCKETINFO* socketInfo, NetworkPacketHeader networkPacketHeader, SerializationBuffer* serializationBuffer);

	// ��� ����鿡�� �۽��ϴ� �Լ�
	void SendBroadcast(NetworkPacketHeader networkPacketHeader,
		SerializationBuffer* serializationBuffer);

	// �濡 �ִ� ����鿡�� �۽��ϴ� �Լ�
	void SendBroadcastRoom(SOCKET exceptSock, NetworkPacketHeader networkPacketHeader, SerializationBuffer* serializationBuffer,
		int roomNo);

	// --------------------------------------------------
	// ��Ŷ�� ����� �Լ���
	// --------------------------------------------------
	// 2. Response �α��� ó�� ��Ŷ�� ����� �Լ�
	void MakePacketResponseLogin(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, BYTE response, WCHAR *id, WCHAR *name);

	// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
	void MakePacketResponseRoomList(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer);

	// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
	void MakePacketResponseRoomCreate(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoStruct);

	// 8. Response ��ȭ�� ���� ó�� �Լ�
	bool MakePacketResponseRoomEnter(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoPtr);

	// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
	int MakePacketResponseChat(NetworkPacketHeader* networkPacketHeader,
		SerializationBuffer* serializationBuffer, SOCKET sock, WORD msgSize, WCHAR* msg);

	// 12. Response �� ���� (����) ó�� �Լ�
	int MakePacketResponseRoomLeave(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, SOCKET sock);

	// 13. Response �� ���� (����) ó�� �Լ�
	void MakePacketResponseRoomDelete(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, int roomNo);

	// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
	void MakePacketResponseUserEnter(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, SOCKET sock, SOCKETINFO* socketInfo);

	// 16. Response ȸ�� ���� ó�� �Լ�
	void MakePacketResponseJoin(NetworkPacketHeader *networkPacketHeader, SerializationBuffer *serializationBuffer, BYTE response, int userNo);

	// 18. Response ȸ�� ���� ���� ó�� �Լ�
	void MakePacketResponseEditInfo(NetworkPacketHeader *networkPacketHeader, SerializationBuffer *serializationBuffer, BYTE response, int userNo);

public:
	~Server();

	void Network(SOCKET listenSock);
};