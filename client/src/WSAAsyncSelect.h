#pragma once

#include "Protocol.h"
#include "SerializationBuffer.h"

// ���� ���� ������ �޽��� ó��
void ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// FD_WRITE ó�� �Լ�
void FDWriteProc();

// FD_READ ó�� �Լ�
void FDReadProc();

// Packet�� ó���ϴ� �Լ�
void PacketProc(WORD msgType, SerializationBuffer* serializationBuffer);

// CheckSum�� ����� �Լ�
int MakeCheckSum(WORD msgType, WORD payloadSize, SerializationBuffer* serializationBuffer);

// CheckSum�� ����� �Լ�
int MakeCheckSum(WORD msgType);

// 1. Request �α��� ó�� �Լ�
void SendRequestLogin(WCHAR *id, WCHAR *pw);

// 2. Response �α��� ó�� �Լ�
void RecvResponseLogin(SerializationBuffer* serializationBuffer);

// 3. Request ��ȭ�� ����Ʈ ó�� �Լ�
void SendRequestRoomList();

// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
void RecvResponseRoomList(SerializationBuffer* serializationBuffer);

// 5. Request ��ȭ�� ���� ó�� �Լ�
void SendRequestRoomCreate();

// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
void RecvResponseRoomCreate(SerializationBuffer* serializationBuffer);

// 7. Request ��ȭ�� ���� ó�� �Լ�
void SendRequestRoomEnter(int index);

// 8. Response ��ȭ�� ���� ó�� �Լ�
void RecvResponseRoomEnter(SerializationBuffer* serializationBuffer);

// 9. Request ä�� �۽� ó�� �Լ�
void SendRequestChat();

// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
void RecvResponseChat(SerializationBuffer* serializationBuffer);

// 11. Request �� ���� ó�� �Լ�
void SendRequestRoomLeave();

// 12. Response �� ���� (����) ó�� �Լ�
void RecvResponseRoomLeave(SerializationBuffer* serializationBuffer);

// 13. Response �� ���� (����) ó�� �Լ�
void RecvResponseRoomDelete(SerializationBuffer* serializationBuffer);

// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
void RecvResponseUserEnter(SerializationBuffer* serializationBuffer);

// 15. Request ȸ�� ���� ��û �Լ�
void SendRequestJoin();

// 16. Response ȸ�� ���� ó�� �Լ�
void RecvResponseJoin(SerializationBuffer *serializationBuffer);

// 17. Request ȸ�� ���� ���� ��û �Լ�
void SendRequestEditInfo();

// 18. Response ȸ�� ���� ���� ó�� �Լ�
void RecvResponseEditInfo(SerializationBuffer *serializationBuffer);

// ������ ��Ŷ�� ������ �Լ�
void SendToServer(NetworkPacketHeader networkPacketHeader);

// ������ ��Ŷ�� ������ �Լ�
void SendToServer(NetworkPacketHeader networkPacketHeader,
	SerializationBuffer* serializationBuffer);

// ���� �Լ� ���� ��� �� ����
void ErrorQuit(WCHAR* msg);

// ���� �Լ� ���� ���
void ErrorDisplay(WCHAR* msg);