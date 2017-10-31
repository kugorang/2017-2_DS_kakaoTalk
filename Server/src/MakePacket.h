#pragma once

// 2. Response �α��� ó�� ��Ŷ�� ����� �Լ�
void MakePacketResponseLogin(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, BYTE response, int userNo);

// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
void MakePacketResponseRoomList(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer);

// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
void MakePacketResponseRoomCreate(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoStruct);

// 8. Response ��ȭ�� ���� ó�� �Լ�
bool MakePacketResponseRoomEnter(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoPtr);

// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
int MakePacketResponseChat(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock, WORD msgSize, WCHAR* msg);

// 12. Response �� ���� (����) ó�� �Լ�
int MakePacketResponseRoomLeave(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock);

// 13. Response �� ���� (����) ó�� �Լ�
void MakePacketResponseRoomDelete(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, int roomNo);

// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
void MakePacketResponseUserEnter(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock, SOCKETINFO* socketInfo);