#include <WinSock2.h>
#include <WS2tcpip.h>
#include <locale.h>
#include <crtdbg.h>
#include "Protocol.h"
#include "main.h"
#include "Select.h"

#pragma comment (lib, "ws2_32.lib")

// ���� ���� ���� ����
static SOCKET listenSock;
map<SOCKET, SOCKETINFO*> socketInfoMap;
list<ROOMINFO*> roomInfoList;

int totalSockets = 0;

int wmain()
{
	//_crtBreakAlloc = 402; // �ߴ����� �ɾ��ִ� ����, _CrtSetDbgFlag�� ��Ʈ
	setlocale(LC_ALL, "");

	int retVal;

	// ���� �ʱ�ȭ
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}

	// socket()
	listenSock = socket(AF_INET, SOCK_STREAM, 0);

	if (listenSock == INVALID_SOCKET)
	{
		ErrorQuit(L"socket()");
	}

	// bind()
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(NETWORK_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	retVal = bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

	if (retVal == SOCKET_ERROR)
	{
		ErrorQuit(L"bind()");
	}

	// listen()
	retVal = listen(listenSock, SOMAXCONN);

	if (retVal == SOCKET_ERROR)
	{
		ErrorQuit(L"listen()");
	}

	wprintf(L"Server Open!\n");

	Network();

	for (map<SOCKET, SOCKETINFO*>::iterator socketInfoMapIter = socketInfoMap.begin();
		socketInfoMapIter != socketInfoMap.end();)
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
			delete[] (*roomInfoListIter)->roomTitle;
			delete (*roomInfoListIter);
			roomInfoListIter = roomInfoList.erase(roomInfoListIter);
		}
		else 
		{
			++roomInfoListIter;
		}
	}

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	return 0;
}

void Network()
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