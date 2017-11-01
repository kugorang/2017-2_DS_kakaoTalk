#include "Server.h"

Server::~Server()
{
	for (map<int, USERINFO*>::iterator userInfoMapIter = userInfoMap.begin(); userInfoMapIter != userInfoMap.end();)
	{
		if (userInfoMapIter->second != nullptr)
		{
			delete userInfoMapIter->second;
			userInfoMapIter = userInfoMap.erase(userInfoMapIter);
		}
		else
		{
			++userInfoMapIter;
		}
	}

	for (map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.begin(); socketInfoMapIter != socketInfoMap.end();)
	{
		if (socketInfoMapIter->second != nullptr)
		{
			socketInfoMapIter = RemoveSocketInfo(socketInfoMapIter->first);
		}
		else
		{
			++socketInfoMapIter;
		}
	}

	for (list<ROOMINFO*>::iterator roomInfoListIter = roomInfoList.begin();
		roomInfoListIter != roomInfoList.end();)
	{
		if ((*roomInfoListIter) != nullptr)
		{
			(*roomInfoListIter)->sockInfo->clear();
			delete[](*roomInfoListIter)->roomTitle;
			delete (*roomInfoListIter);
			roomInfoListIter = roomInfoList.erase(roomInfoListIter);
		}
		else
		{
			++roomInfoListIter;
		}
	}

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}

void Server::Network(SOCKET listenSock)
{
	int retval, addrLen;
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	FD_SET rset, wset;

	do
	{
		map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.begin();
		map<SOCKET, SOCKETINFO*>::iterator checkSockSetIter = socketInfoMap.begin();

		for (int i = 0; i <= totalSockets / 64; i++)
		{
			int socketCount = 1;

			// ���� �� �ʱ�ȭ
			FD_ZERO(&rset);
			FD_ZERO(&wset);

			// listenSock�� �б� �¿� �ִ´�.
			FD_SET(listenSock, &rset);
			map<SOCKET, SOCKETINFO*>::iterator socketInfoMapEnd = socketInfoMap.end();

			// ���� ���� ���� ��� Ŭ���̾�Ʈ���� ������ �б� �¿� �ִ´�.
			for (socketInfoMapIter; socketInfoMapIter != socketInfoMapEnd; ++socketInfoMapIter)
			{
				FD_SET(socketInfoMapIter->first, &rset);

				if (socketInfoMapIter->second->sendQueue.GetUseSize() > 0)
				{
					FD_SET(socketInfoMapIter->first, &wset);
				}

				socketCount++;

				if (socketCount >= 64)
				{
					++socketInfoMapIter;
					socketCount = 1;
					break;
				}
			}

			// select() �Լ��� ȣ���Ѵ�.
			timeval time;
			time.tv_sec = 0;
			time.tv_usec = 0;

			retval = select(0, &rset, &wset, NULL, &time);

			if (retval == SOCKET_ERROR)
			{
				ErrorQuit(L"select()");
			}

			// ���� set �˻� : Ŭ���̾�Ʈ ���� ����
			// select() �Լ��� �����ϸ� ���� �б� set�� �˻��Ͽ�, ������ Ŭ���̾�Ʈ�� �ִ��� Ȯ���Ѵ�.
			// ���� ��� ������ �б� set�� �ִٸ� Ŭ���̾�Ʈ�� ������ ����̴�.
			if (FD_ISSET(listenSock, &rset))
			{
				addrLen = sizeof(clientAddr);

				// accept �Լ��� ȣ���Ѵ�.
				clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);

				// INVALIED_SOCKET�� ���ϵǸ� ������ ȭ�鿡 ����Ѵ�.
				if (clientSock == INVALID_SOCKET)
				{
					ErrorQuit(L"accept()");
				}
				else
				{
					// AddSocketInfo() �Լ��� ȣ���Ͽ� ���� ������ �߰��Ѵ�.
					if (AddSocketInfo(clientSock) == FALSE)
					{
						wprintf(L"Ŭ���̾�Ʈ ������ �����մϴ�!\n");
						closesocket(clientSock);
					}
				}
			}

			// ���� set �˻� : ������ ���
			// select() �Լ��� ������ �����ϴ� ������ ������ ���������� ��ü������ � ��������
			// ���������� �����Ƿ�, �����ϰ� �ִ� ��� ���Ͽ� ���� ���� set�� ��� �ִ��� ���θ� Ȯ���ؾ� �Ѵ�.
			for (checkSockSetIter; checkSockSetIter != socketInfoMapEnd;)
			{
				SOCKET tmpSock = checkSockSetIter->first;
				SOCKETINFO* tmpSocketInfo = checkSockSetIter->second;

				socketCount++;

				if (socketCount > 64)
				{
					++checkSockSetIter;
					socketCount = 1;
					break;
				}

				// ������ �б� �¿� ��� �ִٸ� recv() �Լ��� ȣ���Ͽ� �����͸� �д´�.
				if (FD_ISSET(tmpSock, &rset))
				{
					retval = FDReadProc(tmpSock, tmpSocketInfo);

					if (retval == SOCKET_ERROR)
					{
						checkSockSetIter = RemoveSocketInfo(tmpSock);
						continue;
					}
				}

				// ������ ���� �¿� ��� �ִٸ� send() �Լ��� ȣ���Ͽ� �����͸� ����.
				if (FD_ISSET(tmpSock, &wset))
				{
					retval = FDWriteProc(tmpSock, tmpSocketInfo);

					if (retval == SOCKET_ERROR)
					{
						checkSockSetIter = RemoveSocketInfo(tmpSock);
						continue;
					}
				}

				++checkSockSetIter;
			}
		}
	} while (!((GetAsyncKeyState(VK_DOWN) & 0x8000) && (GetAsyncKeyState(VK_SPACE) & 0x8000)));
}

int Server::FDWriteProc(SOCKET sock, SOCKETINFO* socketInfo)
{
	int retval, useSize, notBrokenGetSize;
	RingBuffer* sendQueue = &(socketInfo->sendQueue);

	while (true)
	{
		useSize = sendQueue->GetUseSize();
		notBrokenGetSize = sendQueue->GetNotBrokenGetSize();

		if (useSize <= 0)
		{
			return 0;
		}

		retval = send(sock, (char*)sendQueue->GetFrontPosBufferPtr(), notBrokenGetSize, 0);

		if (retval == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}
		else
		{
			sendQueue->MoveFrontPos(retval);
		}
	}
}

int Server::FDReadProc(SOCKET sock, SOCKETINFO* socketInfo)
{
	int retval, useSize, remainSize, notBrokenPutSize;
	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;
	RingBuffer* recvQueue = &(socketInfo->recvQueue);

	while (true)
	{
		remainSize = recvQueue->GetRemainSize();
		notBrokenPutSize = recvQueue->GetNotBrokenPutSize();

		retval = recv(sock, (char*)recvQueue->GetRearPosBufferPtr(), notBrokenPutSize, 0);

		if (retval == SOCKET_ERROR || retval == 0)
		{
			return SOCKET_ERROR;
		}
		else
		{
			recvQueue->MoveRearPos(retval);
		}

		while (true)
		{
			useSize = recvQueue->GetUseSize();

			if (sizeof(networkPacketHeader) > useSize)
			{
				return 0;
			}

			recvQueue->Peek((BYTE*)&networkPacketHeader, sizeof(networkPacketHeader));

			if ((int)(sizeof(networkPacketHeader) + networkPacketHeader.PayloadSize) > useSize)
			{
				return 0;
			}

			if (networkPacketHeader.code != NETWORK_PACKET_CODE)
			{
				return SOCKET_ERROR;
			}

			recvQueue->MoveFrontPos(sizeof(networkPacketHeader));

			retval = recvQueue->Dequeue(serializationBuffer.GetRearPosBufferPtr(),
				networkPacketHeader.PayloadSize);

			serializationBuffer.MoveRearPos(retval);

			if (MakeCheckSum(networkPacketHeader.MsgType, networkPacketHeader.PayloadSize,
				&serializationBuffer) == networkPacketHeader.checkSum)
			{
				PacketProc(sock, networkPacketHeader.MsgType, &serializationBuffer);
			}
			else
			{
				wprintf(L"CheckSum ���� - ");
				RemoveSocketInfo(sock);
			}
		}
	}
}

// CheckSum�� ����� �Լ�
int Server::MakeCheckSum(WORD msgType, WORD payloadSize, SerializationBuffer* serializationBuffer)
{
	BYTE* confirmCheckSum = new BYTE[payloadSize];

	serializationBuffer->Peek(confirmCheckSum, payloadSize);

	WORD byteSum = 0;

	for (int i = 0; i < 2; i++)
	{
		byteSum += *((BYTE*)&msgType + i);
	}

	for (int i = 0; i < payloadSize; i++)
	{
		byteSum += *(confirmCheckSum + i);
	}

	delete[] confirmCheckSum;

	return byteSum % 256;
}

// Packet�� ó���ϴ� �Լ�
void Server::PacketProc(SOCKET sock, WORD type, SerializationBuffer* serializationBuffer)
{
	switch (type)
	{
	case REQUSET_LOGIN:
		RecvRequestLogin(sock, serializationBuffer);
		break;
	case REQUEST_ROOM_LIST:
		RecvRequestRoomList(sock, serializationBuffer);
		break;
	case REQUEST_ROOM_CREATE:
		RecvRequestRoomCreate(sock, serializationBuffer);
		break;
	case REQUEST_ROOM_ENTER:
		RecvRequestRoomEnter(sock, serializationBuffer);
		break;
	case REQUEST_CHAT:
		RecvRequestChat(sock, serializationBuffer);
		break;
	case REQUEST_ROOM_LEAVE:
		RecvRequestRoomLeave(sock, serializationBuffer);
		break;
	case REQUEST_JOIN:
		RecvRequestJoin(sock, serializationBuffer);
		break;
	case REQUEST_EDIT_INFO:
		RecvRequestEditInfo(sock, serializationBuffer);
		break;
	default:
		wprintf(L"�޼��� Ÿ�� ���� - ");
		RemoveSocketInfo(sock);
		break;
	}
}

// 1. Request �α��� ó�� �Լ�
void Server::RecvRequestLogin(SOCKET sock, SerializationBuffer* serializationBuffer)
{
	wprintf(L"Recv : 01 - �α��� ��û [SOCK : %lld]\n", sock);

	WCHAR id[ID_MAX_LEN], pw[PW_MAX_LEN];

	*serializationBuffer >> id >> pw;

	SendResponseLogin(sock, id, pw);
}

// 2. Response �α��� ó�� �Լ�
void Server::SendResponseLogin(SOCKET sock, WCHAR* id, WCHAR *pw)
{
	map<SOCKET, SOCKETINFO*>::iterator sockInfoMapIter = socketInfoMap.find(sock);
	SOCKETINFO* sockInfo = sockInfoMapIter->second;

	wprintf(L"Send : 02 - �α��� ���� [SOCK : %lld]\n", sock);

	BYTE response = RESPONSE_LOGIN_ID_ERR;
	WCHAR idBuf[ID_MAX_LEN];
	WCHAR nameBuf[NAME_MAX_LEN];

	map<int, USERINFO*>::iterator userInfoMapIter;
	map<int, USERINFO*>::iterator userInfoMapEnd = userInfoMap.end();

	for (userInfoMapIter = userInfoMap.begin(); userInfoMapIter != userInfoMapEnd; ++userInfoMapIter)
	{
		USERINFO *userInfo = userInfoMapIter->second;

		// ���̵� ���� ��
		if (wcscmp(userInfo->id, id) == 0)
		{
			// ��й�ȣ���� ���ٸ�
			if (wcscmp(userInfo->pw, pw) == 0)
			{
				map<SOCKET, SOCKETINFO*>::iterator socketInfoMapEnd = socketInfoMap.end();

				for (sockInfoMapIter = socketInfoMap.begin(); sockInfoMapIter != socketInfoMapEnd; ++sockInfoMapIter)
				{
					if ((*sockInfoMapIter).second->userInfo == userInfo)
					{
						response = RESPONSE_LOGIN_ALREADY;
						break;
					}
				}

				if (sockInfoMapIter == socketInfoMapEnd)
				{
					// �α��� ����
					response = RESPONSE_LOGIN_OK;
					sockInfo->userInfo = userInfo;
					wcscpy_s(idBuf, sizeof(idBuf), userInfo->id);
					wcscpy_s(nameBuf, sizeof(nameBuf), userInfo->name);
					break;
				}
			}
			else
			{
				// ��й�ȣ�� �ٸ��� �˸�.
				response = RESPONSE_LOGIN_PW_ERR;
				break;
			}
		}
	}

	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	MakePacketResponseLogin(&networkPacketHeader, &serializationBuffer, response, idBuf, nameBuf);

	SendUnicast(sock, networkPacketHeader, &serializationBuffer);
}

// 3. Request ��ȭ�� ����Ʈ ó�� �Լ�
void Server::RecvRequestRoomList(SOCKET sock, SerializationBuffer* serializationBuffer)
{
	wprintf(L"Recv : 03 - �� ����Ʈ ��û [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	SendResponseRoomList(sock);
}

// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
void Server::SendResponseRoomList(SOCKET sock)
{
	wprintf(L"Send : 04 - �� ����Ʈ ���� [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	MakePacketResponseRoomList(&networkPacketHeader, &serializationBuffer);

	SendUnicast(sock, networkPacketHeader, &serializationBuffer);
}

// 5. Request ��ȭ�� ���� ó�� �Լ�
void Server::RecvRequestRoomCreate(SOCKET sock, SerializationBuffer* serializationBuffer)
{
	wprintf(L"Recv : 05 - �� ���� ��û [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	WORD roomTitleSize;

	*serializationBuffer >> roomTitleSize;

	WCHAR* roomTitle = new WCHAR[roomTitleSize / 2 + 1];

	serializationBuffer->Dequeue((BYTE*)roomTitle, roomTitleSize + 2);

	roomTitle[roomTitleSize / 2] = '\0';

	SendResponseRoomCreate(sock, roomTitle, roomTitleSize + 2);
}

// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
void Server::SendResponseRoomCreate(SOCKET sock, WCHAR* roomTitle, WORD roomTitleSize)
{
	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	static WORD roomNum = 1;
	BYTE response = RESPONSE_ROOM_CREATE_OK;

	// �� �̸� �ߺ� �˻�
	for (list<ROOMINFO*>::iterator iter = roomInfoList.begin(); iter != roomInfoList.end(); ++iter)
	{
		if (wcscmp((*iter)->roomTitle, roomTitle) == 0)
		{
			response = RESPONSE_ROOM_CREATE_DNICK;
			break;
		}
	}

	// �� ���� �ʰ� Ȯ��
	if (totalRooms >= ROOM_MAX)
	{
		response = RESPONSE_ROOM_CREATE_MAX;
	}

	wprintf(L"Send : 06 - �� ���� ���� [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	ROOMINFO* roomInfoStruct = new ROOMINFO;

	roomInfoStruct->roomNo = roomNum++;
	roomInfoStruct->roomTitleSize = roomTitleSize;
	roomInfoStruct->roomTitle = roomTitle;
	roomInfoStruct->numberOfPeople = 0;

	MakePacketResponseRoomCreate(&networkPacketHeader, &serializationBuffer,
		response, roomInfoStruct);

	if (response == RESPONSE_ROOM_CREATE_OK)
	{
		totalRooms++;
		roomInfoList.push_back(roomInfoStruct);

		SendBroadcast(networkPacketHeader, &serializationBuffer);
	}
	else
	{
		SendUnicast(sock, networkPacketHeader, &serializationBuffer);
	}

}

// 7. Request ��ȭ�� ���� ó�� �Լ�
void Server::RecvRequestRoomEnter(SOCKET sock, SerializationBuffer* serializationBuffer)
{
	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);

	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	wprintf(L"Recv : 07 - �� ���� ��û [NO : %d]\n", tmpSocketInfo->userInfo->userNo);

	int roomNum;

	*serializationBuffer >> roomNum;

	BYTE response = RESPONSE_ROOM_ENTER_OK;

	// �� ��ȣ ���� �˻�
	list<ROOMINFO*>::iterator roomInfoListIter;
	list<ROOMINFO*>::iterator roomInfoListEnd = roomInfoList.end();

	for (roomInfoListIter = roomInfoList.begin(); roomInfoListIter != roomInfoListEnd;
		++roomInfoListIter)
	{
		if ((*roomInfoListIter)->roomNo == roomNum)
		{
			break;
		}
	}

	if (roomInfoListIter == roomInfoListEnd || tmpSocketInfo->alreadyRoom)
	{
		response = RESPONSE_ROOM_ENTER_NOT;
	}

	// �ο� �ʰ�
	if (!(roomInfoListIter == roomInfoListEnd) && (*roomInfoListIter)->numberOfPeople >= ROOM_PEOPLE_MAX)
	{
		response = RESPONSE_ROOM_ENTER_MAX;
	}

	SendResponseRoomEnter(sock, response, (*roomInfoListIter));
}

// 8. Response ��ȭ�� ���� ó�� �Լ�
void Server::SendResponseRoomEnter(SOCKET sock, BYTE response, ROOMINFO* roomInfoPtr)
{
	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);
	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	if (response == RESPONSE_ROOM_ENTER_OK)
	{
		wprintf(L"Send : 08 - �� ���� ���� [NO : %d]\n", tmpSocketInfo->userInfo->userNo);

		roomInfoPtr->sockInfo->push_back(tmpSocketInfo);

		tmpSocketInfo->alreadyRoom = true;
		tmpSocketInfo->enterRoomNo = roomInfoPtr->roomNo;

		(roomInfoPtr->numberOfPeople)++;
	}

	if (MakePacketResponseRoomEnter(&networkPacketHeader, &serializationBuffer, response, roomInfoPtr))
	{
		SendUnicast(sock, networkPacketHeader, &serializationBuffer);

		SendResponseUserEnter(sock, tmpSocketInfo);
	}
}

// 9. Request ä�� �۽� ó�� �Լ�
void Server::RecvRequestChat(SOCKET sock, SerializationBuffer* serializationBuffer)
{
	WORD msgSize;

	wprintf(L"Recv : 09 - ä�� �۽� [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	*serializationBuffer >> msgSize;

	WCHAR* msg = new WCHAR[msgSize / 2 + 1];

	serializationBuffer->Dequeue((BYTE*)msg, msgSize + 2);

	msg[msgSize / 2] = '\0';

	SendResponseChat(sock, msgSize + 2, msg);

	delete[] msg;
}

// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
void Server::SendResponseChat(SOCKET sock, WORD msgSize, WCHAR* msg)
{
	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	int roomNo = MakePacketResponseChat(&networkPacketHeader, &serializationBuffer, sock, msgSize, msg);

	SendBroadcastRoom(sock, networkPacketHeader, &serializationBuffer, roomNo);
}

// 11. Request �� ���� ó�� �Լ�
void Server::RecvRequestRoomLeave(SOCKET sock, SerializationBuffer* serializationBuffer)
{
	wprintf(L"Recv : 11 - �� ���� ��û [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	SendResponseRoomLeave(sock);
}

// 12. Response �� ���� (����) ó�� �Լ�
void Server::SendResponseRoomLeave(SOCKET sock)
{
	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	int roomNo = MakePacketResponseRoomLeave(&networkPacketHeader, &serializationBuffer, sock);

	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);
	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	tmpSocketInfo->alreadyRoom = false;
	tmpSocketInfo->enterRoomNo = -1;

	SendBroadcast(networkPacketHeader, &serializationBuffer);

	ROOMINFO* tmpRoomInfo = FindRoom(roomNo);

	if (tmpRoomInfo == nullptr)
	{
		wprintf(L"���� ã�� ���Ͽ����ϴ�. (12)\n");
		return;
	}

	list<SOCKETINFO*>* sockInfo = tmpRoomInfo->sockInfo;
	list<SOCKETINFO*>::iterator sockInfoIter;
	list<SOCKETINFO*>::iterator sockInfoEnd = sockInfo->end();

	for (sockInfoIter = sockInfo->begin(); sockInfoIter != sockInfoEnd; ++sockInfoIter)
	{
		if ((*sockInfoIter) == tmpSocketInfo)
		{
			sockInfo->erase(sockInfoIter);
			break;
		}
	}

	(tmpRoomInfo->numberOfPeople)--;

	if (tmpRoomInfo->numberOfPeople == 0)
	{
		SendResponseRoomDelete(sock, roomNo);
	}
}

// 13. Response �� ���� (����) ó�� �Լ�
void Server::SendResponseRoomDelete(SOCKET sock, int roomNo)
{
	wprintf(L"Send : 13 - �� ���� ���� [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	MakePacketResponseRoomDelete(&networkPacketHeader, &serializationBuffer, roomNo);

	ROOMINFO* tmpRoomInfo = FindRoom(roomNo);

	if (tmpRoomInfo != nullptr)
	{
		delete[] tmpRoomInfo->roomTitle;
		delete tmpRoomInfo;

		for (list<ROOMINFO*>::iterator roomInfoListIter = roomInfoList.begin();
			roomInfoListIter != roomInfoList.end(); ++roomInfoListIter)
		{
			if ((*roomInfoListIter) == tmpRoomInfo)
			{
				roomInfoList.erase(roomInfoListIter);
				break;
			}
		}
	}

	totalRooms--;

	SendBroadcast(networkPacketHeader, &serializationBuffer);
}

// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
void Server::SendResponseUserEnter(SOCKET sock, SOCKETINFO* socketInfo)
{
	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	MakePacketResponseUserEnter(&networkPacketHeader, &serializationBuffer, sock, socketInfo);

	SendBroadcastRoom(sock, networkPacketHeader, &serializationBuffer, socketInfo->enterRoomNo);
}

// 15. Request ȸ�� ���� ó�� �Լ�
void Server::RecvRequestJoin(SOCKET sock, SerializationBuffer *serializationBuffer)
{
	wprintf(L"Recv : 15 - ȸ�� ���� ��û [SOCK : %lld]\n", sock);

	WCHAR id[ID_MAX_LEN], pw[PW_MAX_LEN], name[NAME_MAX_LEN], phoneNum[PHONENUM_MAX_LEN];

	*serializationBuffer >> id >> pw >> name >> phoneNum;

	SendResponseJoin(sock, id, pw, name, phoneNum);
}

// 16. Response ȸ�� ���� ó�� �Լ�
void Server::SendResponseJoin(SOCKET sock, WCHAR *id, WCHAR *pw, WCHAR *name, WCHAR *phoneNum)
{
	wprintf(L"Send : 16 - ȸ�� ���� ���� [SOCK : %lld]\n", sock);

	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	BYTE response = RESPONSE_JOIN_OK;

	// ���̵� �ߺ� �˻�.
	map<int, USERINFO*>::iterator userInfoMapIter;

	for (userInfoMapIter = userInfoMap.begin(); userInfoMapIter != userInfoMap.end(); ++userInfoMapIter)
	{
		if (wcscmp(userInfoMapIter->second->id, id) == 0)
		{
			response = RESPONSE_JOIN_DNICK;
			break;
		}
	}

	// ����� �ʰ� Ȯ��
	if (totalSockets >= SOCKETINFO_ARRAY_MAX)
	{
		response = RESPONSE_JOIN_MAX;
	}

	static int userNo = 1;

	if (response == RESPONSE_LOGIN_OK)
	{
		USERINFO *newUser = new USERINFO;

		newUser->userNo = userNo++;
		wcscpy_s(newUser->id, ID_MAX_LEN, id);
		wcscpy_s(newUser->pw, PW_MAX_LEN, pw);
		wcscpy_s(newUser->name, NAME_MAX_LEN, name);
		wcscpy_s(newUser->phoneNum, PHONENUM_MAX_LEN, phoneNum);

		userInfoMap.insert(map<int, USERINFO*>::value_type(userNo, newUser));
	}

	MakePacketResponseJoin(&networkPacketHeader, &serializationBuffer, response, userNo);

	SendUnicast(sock, networkPacketHeader, &serializationBuffer);
}

// 17. Request ȸ�� ���� ���� �Լ�
void Server::RecvRequestEditInfo(SOCKET sock, SerializationBuffer *serializationBuffer)
{
	wprintf(L"Recv : 17 - ȸ�� ���� ���� ��û [NO : %d]\n", socketInfoMap.find(sock)->second->userInfo->userNo);

	WCHAR id[ID_MAX_LEN], pw[PW_MAX_LEN], name[NAME_MAX_LEN], phoneNum[PHONENUM_MAX_LEN];

	*serializationBuffer >> id >> pw >> name >> phoneNum;

	SendResponseEditInfo(sock, id, pw, name, phoneNum);
}

// 18. Response ȸ�� ���� ���� �Լ�
void Server::SendResponseEditInfo(SOCKET sock, WCHAR *id, WCHAR *pw, WCHAR *name, WCHAR *phoneNum)
{
	USERINFO *editUser = socketInfoMap.find(sock)->second->userInfo;
	wprintf(L"Send : 16 - ȸ�� ���� ���� ���� [NO : %d]\n", editUser->userNo);

	NetworkPacketHeader networkPacketHeader;
	SerializationBuffer serializationBuffer;

	BYTE response = RESPONSE_EDIT_INFO_OK;
	int userNo = -1;

	// ���̵� �ߺ� �˻�.
	map<int, USERINFO*>::iterator userInfoMapIter;

	for (userInfoMapIter = userInfoMap.begin(); userInfoMapIter != userInfoMap.end(); ++userInfoMapIter)
	{
		if (wcscmp(userInfoMapIter->second->id, id) == 0)
		{
			response = RESPONSE_EDIT_INFO_DNICK;
			break;
		}
	}

	if (response == RESPONSE_LOGIN_OK)
	{
		userNo = editUser->userNo;
		wcscpy_s(editUser->id, ID_MAX_LEN, id);
		wcscpy_s(editUser->pw, PW_MAX_LEN, pw);
		wcscpy_s(editUser->name, NAME_MAX_LEN, name);
		wcscpy_s(editUser->phoneNum, PHONENUM_MAX_LEN, phoneNum);
	}

	MakePacketResponseEditInfo(&networkPacketHeader, &serializationBuffer, response, userNo);

	SendUnicast(sock, networkPacketHeader, &serializationBuffer);
}

// ���� ������ �߰��ϴ� �Լ��̴�.
BOOL Server::AddSocketInfo(SOCKET sock)
{
	if (totalSockets > SOCKETINFO_ARRAY_MAX)
	{
		wprintf(L"[����] ���� ������ �߰��� �� �����ϴ�!\n");

		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;

	ptr->alreadyRoom = false;
	ptr->enterRoomNo = -1;

	wprintf(L"���� ���� [SOCK : %lld]\n", sock);

	socketInfoMap.insert(map<SOCKET, SOCKETINFO*>::value_type(sock, ptr));
	//socketInfoMap[sock] = ptr; // -> �޸𸮸� �߰� �Ҵ��ϱ� ������ ������� �� ��.

	totalSockets++;

	return TRUE;
}

// ���� ������ �����ϴ� �Լ��̴�.
map<SOCKET, SOCKETINFO*>::iterator Server::RemoveSocketInfo(SOCKET sock)
{
	map<SOCKET, SOCKETINFO*>::iterator iter = socketInfoMap.find(sock);
	SOCKETINFO *sockInfo = iter->second;

	wprintf(L"���� ���� [SOCK : %lld]\n", sock);

	if (iter != socketInfoMap.end())
	{
		if (sockInfo->enterRoomNo != -1)
		{
			SendResponseRoomLeave(sock);
		}

		delete sockInfo;

		closesocket(sock);

		iter = socketInfoMap.erase(iter);
		totalSockets--;
	}

	return iter;
}

// ���� ã�� �Լ�
ROOMINFO* Server::FindRoom(int roomNo)
{
	list<ROOMINFO*>::iterator roomInfoListIter;

	for (roomInfoListIter = roomInfoList.begin(); roomInfoListIter != roomInfoList.end();	++roomInfoListIter)
	{
		if ((*roomInfoListIter)->roomNo == roomNo)
		{
			return (*roomInfoListIter);
		}
	}

	return nullptr;
}

// �� ������Ը� �۽��ϴ� �Լ�
void Server::SendUnicast(SOCKET sock, NetworkPacketHeader networkPacketHeader,
	SerializationBuffer* serializationBuffer)
{
	map<SOCKET, SOCKETINFO*>::iterator iter = socketInfoMap.find(sock);

	iter->second->sendQueue.Enqueue((BYTE*)&networkPacketHeader, sizeof(networkPacketHeader));
	iter->second->sendQueue.Enqueue(serializationBuffer->GetFrontPosBufferPtr(),
		serializationBuffer->GetUseSize());
}

// �� ������Ը� �۽��ϴ� �Լ�
void Server::SendUnicast(SOCKETINFO* socketInfo, NetworkPacketHeader networkPacketHeader, SerializationBuffer* serializationBuffer)
{
	socketInfo->sendQueue.Enqueue((BYTE*)&networkPacketHeader, sizeof(networkPacketHeader));
	socketInfo->sendQueue.Enqueue(serializationBuffer->GetFrontPosBufferPtr(),
		serializationBuffer->GetUseSize());
}

// ��� ����鿡�� �۽��ϴ� �Լ�
void Server::SendBroadcast(NetworkPacketHeader networkPacketHeader, SerializationBuffer* serializationBuffer)
{
	map<SOCKET, SOCKETINFO*>::iterator iter;

	for (iter = socketInfoMap.begin(); iter != socketInfoMap.end(); ++iter)
	{
		SendUnicast(iter->first, networkPacketHeader, serializationBuffer);
	}
}

// �濡 �ִ� ����鿡�� �۽��ϴ� �Լ�
void Server::SendBroadcastRoom(SOCKET exceptSock, NetworkPacketHeader networkPacketHeader,
	SerializationBuffer* serializationBuffer, int roomNo)
{
	ROOMINFO* tmpRoomInfo = FindRoom(roomNo);

	if (tmpRoomInfo == nullptr)
	{
		wprintf(L"���� ã�� ���Ͽ����ϴ� (SBR).\n");
		return;
	}

	list<SOCKETINFO*>* sockInfo = tmpRoomInfo->sockInfo;

	list<SOCKETINFO*>::iterator sockInfoListIter;

	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(exceptSock);
	int exceptSockUserNo = socketInfoMapIter->second->userInfo->userNo;

	for (sockInfoListIter = sockInfo->begin(); sockInfoListIter != sockInfo->end(); ++sockInfoListIter)
	{
		if ((*sockInfoListIter)->userInfo->userNo != exceptSockUserNo)
		{
			SendUnicast(*sockInfoListIter, networkPacketHeader, serializationBuffer);
		}
	}
}

// ���� �Լ� ���� ��� �� ����
void Server::ErrorQuit(WCHAR* msg)
{
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
}

// 2. Response �α��� ó�� ��Ŷ�� ����� �Լ�
void Server::MakePacketResponseLogin(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, BYTE response, WCHAR *id, WCHAR *name)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_LOGIN;

	*serializationBuffer << response;
	serializationBuffer->Enqueue((BYTE*)id, ID_MAX_LEN * sizeof(WCHAR));
	serializationBuffer->Enqueue((BYTE*)name, NAME_MAX_LEN * sizeof(WCHAR));

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}

// 4. Response ��ȭ�� ����Ʈ ó�� �Լ�
void Server::MakePacketResponseRoomList(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer)
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
			*serializationBuffer << (*sockInfoIter)->userInfo->name;
		}
	}

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}

// 6. Response ��ȭ�� ���� (����) ó�� �Լ�
void Server::MakePacketResponseRoomCreate(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoStruct)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_CREATE;

	WORD roomTitleSize = roomInfoStruct->roomTitleSize;

	*serializationBuffer << response << roomInfoStruct->roomNo << roomTitleSize;

	serializationBuffer->Enqueue((BYTE*)roomInfoStruct->roomTitle, roomTitleSize);

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();

	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}

// 8. Response ��ȭ�� ���� ó�� �Լ�
bool Server::MakePacketResponseRoomEnter(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, BYTE response, ROOMINFO* roomInfoPtr)
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
			*serializationBuffer << (*socketInfoIter)->userInfo->name << (*socketInfoIter)->userInfo->userNo;
		}

		networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();

		networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);

		return true;
	}

	return false;
}

// 10. Response ä�� ���� (����) (������ ���� ����) ó�� �Լ�
int Server::MakePacketResponseChat(NetworkPacketHeader* networkPacketHeader,
	SerializationBuffer* serializationBuffer, SOCKET sock, WORD msgSize, WCHAR* msg)
{
	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);

	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	wprintf(L"Send : 10 - ä�� ���� [NO : %d]\n", tmpSocketInfo->userInfo->userNo);

	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_CHAT;

	*serializationBuffer << tmpSocketInfo->userInfo->userNo << msgSize;

	serializationBuffer->Enqueue((BYTE*)msg, msgSize);

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);

	return tmpSocketInfo->enterRoomNo;
}

// 12. Response �� ���� (����) ó�� �Լ�
int Server::MakePacketResponseRoomLeave(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, SOCKET sock)
{
	map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.find(sock);

	SOCKETINFO* tmpSocketInfo = socketInfoMapIter->second;

	int userNo = tmpSocketInfo->userInfo->userNo;

	wprintf(L"Send : 12 - �� ���� ���� [NO : %d]\n", userNo);

	*serializationBuffer << userNo;

	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_LEAVE;
	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);

	return tmpSocketInfo->enterRoomNo;
}

// 13. Response �� ���� (����) ó�� �Լ�
void Server::MakePacketResponseRoomDelete(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, int roomNo)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_ROOM_DELETE;

	*serializationBuffer << roomNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}

// 14. Response Ÿ ����� ���� (����) ó�� �Լ�
void Server::MakePacketResponseUserEnter(NetworkPacketHeader* networkPacketHeader, SerializationBuffer* serializationBuffer, SOCKET sock, SOCKETINFO* socketInfo)
{
	int userNo = socketInfo->userInfo->userNo;

	wprintf(L"Send : 14 - �ٸ� ���� ���� ���� [NO : %d]\n", userNo);

	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_USER_ENTER;

	serializationBuffer->Enqueue((BYTE*)socketInfo->userInfo->name, sizeof(WCHAR) * ID_MAX_LEN);
	*serializationBuffer << userNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}

// 16. Response ȸ�� ���� ó�� �Լ�
void Server::MakePacketResponseJoin(NetworkPacketHeader *networkPacketHeader, SerializationBuffer *serializationBuffer, BYTE response, int userNo)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_JOIN;

	*serializationBuffer << response << userNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}

// 18. Response ȸ�� ���� ���� ó�� �Լ�
void Server::MakePacketResponseEditInfo(NetworkPacketHeader *networkPacketHeader, SerializationBuffer *serializationBuffer, BYTE response, int userNo)
{
	networkPacketHeader->code = NETWORK_PACKET_CODE;
	networkPacketHeader->MsgType = RESPONSE_EDIT_INFO;

	*serializationBuffer << response << userNo;

	networkPacketHeader->PayloadSize = serializationBuffer->GetUseSize();
	networkPacketHeader->checkSum = MakeCheckSum(networkPacketHeader->MsgType, networkPacketHeader->PayloadSize, serializationBuffer);
}