#include <WinSock2.h>
#include <WS2tcpip.h>
#include <locale.h>
#include <crtdbg.h>
#include "Application.h"

void Application::Run()
{
	//_crtBreakAlloc = 402; // �ߴ����� �ɾ��ִ� ����, _CrtSetDbgFlag�� ��Ʈ
	setlocale(LC_ALL, "");

	int retVal;

	// ���� �ʱ�ȭ
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return;
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

	server.Network(listenSock);
}

// ���� �Լ� ���� ��� �� ����
void Application::ErrorQuit(WCHAR* msg)
{
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
}