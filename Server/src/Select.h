#pragma once

#include "SerializationBuffer.h"

// ���� �Լ� ���� ��� �� ����
void ErrorQuit(WCHAR* msg);

// ���� �Լ� ���� ���
void ErrorDisplay(WCHAR* msg);

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
void SendResponseLogin(SOCKET sock, WCHAR* nickname);

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

// ���� ���� �Լ�
BOOL AddSocketInfo(SOCKET sock);
map<SOCKET, SOCKETINFO*>::iterator RemoveSocketInfo(SOCKET sock);

// ���� ã�� �Լ�
ROOMINFO* FindRoom(int roomNo);

// �� ������Ը� �۽��ϴ� �Լ�
void SendUnicast(SOCKET sock, 
	NetworkPacketHeader networkPacketHeader, 
	SerializationBuffer* serializationBuffer);

// ��� ����鿡�� �۽��ϴ� �Լ�
void SendBroadcast(NetworkPacketHeader networkPacketHeader, 
	SerializationBuffer* serializationBuffer);

// �濡 �ִ� ����鿡�� �۽��ϴ� �Լ�
void SendBroadcastRoom(SOCKET exceptSock, 
	NetworkPacketHeader networkPacketHeader,
	SerializationBuffer* serializationBuffer,
	int roomNo);