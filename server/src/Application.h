#pragma once

#include "Server.h"

class Application
{
private:
	// ����
	Server server;

	// listen ����
	SOCKET listenSock;

	// ���� �Լ� ���� ��� �� ����
	void ErrorQuit(WCHAR* msg);

public:
	void Run();
};