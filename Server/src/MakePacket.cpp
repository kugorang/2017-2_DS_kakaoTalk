#include "Protocol.h"
#include "main.h"
#include "Select.h"
#include "MakePacket.h"

// 2. Response �α��� ó�� ��Ŷ�� ����� �Լ�
void MakePacketResponseLogin(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, BYTE response, int userNo)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_LOGIN;	

	*serializationBuffer << response << userNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType,
		networkPacketHeader->PayloadSize, serializationBuffer);
}

// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
void MakePacketResponseRoomList(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_LIST;

	list<ROOMINFO*>::size_type roomNum = roomInfoList.size();

	*serializationBuffer << (WORD)roomNum;

	for (list<ROOMINFO*>::iterator iter = roomInfoList.begin(); iter != roomInfoList.end(); ++iter)
	{
		int roomNo = (*iter)->roomNo;
		WORD roomTitleSize = (*iter)->roomTitleSize;
		WCHAR* roomTitle = (*iter)->roomTitle;
		BYTE* numberOfPeople = &((*iter)->numberOfPeople);
		list<SOCKETINFO*>* sockInfo = (*iter)->sockInfo;

		*serializationBuffer << roomNo << roomTitleSize;

		serializationBuffer->Enqueue((BYTE*)roomTitle, roomTitleSize);

		*serializationBuffer << *numberOfPeople;

		list<SOCKETINFO*>::iterator sockInfoIter;

		for (sockInfoIter = sockInfo->begin(); sockInfoIter != sockInfo->end(); ++sockInfoIter)
		{
			*serializationBuffer << (*sockInfoIter)->nickname;
		}
	}

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType,
		networkPacketHeader->PayloadSize, serializationBuffer);
}

// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
void MakePacketResponseRoomCreate(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoStruct)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_CREATE;

	WORD roomTitleSize = roomInfoStruct->roomTitleSize;
	
	*serializationBuffer << response << roomInfoStruct->roomNo << roomTitleSize;

	serializationBuffer->Enqueue((BYTE*)roomInfoStruct->roomTitle, roomTitleSize);

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();

	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType,
		networkPacketHeader->PayloadSize, serializationBuffer);
}

// 8. Response ��ȭ�� ���� ó�� �Լ�
bool MakePacketResponseRoomEnter(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoPtr)
{
	if (response == RESPONSE_ROOM_ENTER_OK)
	{
		WORD roomTitleSize = roomInfoPtr->roomTitleSize;
		list<SOCKETINFO*>* sockInfo = roomInfoPtr->sockInfo;

		networkPacketHeader->code = NETWORK_PACKET_CODE;
		networkPacketHeader->MsgType = RESPONSE_ROOM_ENTER;

		*serializationBuffer << response << roomInfoPtr->roomNo << roomTitleSize;

		serializationBuffer->Enqueue((BYTE*)roomInfoPtr->roomTitle, roomTitleSize);

		*serializationBuffer << roomInfoPtr->numberOfPeople;

		list<SOCKETINFO*>::iterator socketInfoIter;

		for (socketInfoIter = sockInfo->begin(); socketInfoIter != sockInfo->end(); ++socketInfoIter)
		{
			*serializationBuffer << (*socketInfoIter)->nickname << (*socketInfoIter)->userNo;
		}

		networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();

		networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType,
			networkPacketHeader->PayloadSize, serializationBuffer);

		return true;
	}

	return false;
}

// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
int MakePacketResponseChat(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock, WORD msgSize, WCHAR* msg)
{
	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);

	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	wprintf(L"Send : 10 - ä�� ���� [NO : %d]\n", tmpSocketInfo->userNo);

	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_CHAT;

	*serializationBuffer << tmpSocketInfo->userNo << msgSize;

	serializationBuffer->Enqueue((BYTE*)msg, msgSize);

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, 
		networkPacketHeader->PayloadSize, serializationBuffer);

	return tmpSocketInfo->enterRoomNo;
}

// 12. Response �� ���� (����) ó�� �Լ�
int MakePacketResponseRoomLeave(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock)
{
	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);

	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	int userNo = tmpSocketInfo->userNo;

	wprintf(L"Send : 12 - �� ���� ���� [NO : %d]\n", userNo);
	
	*serializationBuffer << userNo;

	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_LEAVE;				
	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, 
		networkPacketHeader->PayloadSize, serializationBuffer);

	return tmpSocketInfo->enterRoomNo;
}

// 13. Response �� ���� (����) ó�� �Լ�
void MakePacketResponseRoomDelete(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, int roomNo)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_DELETE;

	*serializationBuffer << roomNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, 
		networkPacketHeader->PayloadSize, serializationBuffer);
}

// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
void MakePacketResponseUserEnter(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock, SOCKETINFO* socketInfo)
{
	int userNo = socketInfo->userNo;

	wprintf(L"Send : 14 - �ٸ� ���� ���� ���� [NO : %d]\n", userNo);
	
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_USER_ENTER;

	serializationBuffer->Enqueue((BYTE*)socketInfo->nickname, sizeof(WCHAR) * NICK_MAX_LEN);
	*serializationBuffer << userNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType,
		networkPacketHeader->PayloadSize, serializationBuffer);
}