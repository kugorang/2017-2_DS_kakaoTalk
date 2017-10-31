#pragma once

#include <Windows.h>
#include <map>
#include <list>
#include "RingBuffer.h"

using namespace std;

#define SOCKETINFO_ARRAY_MAX 100
#define ROOM_MAX 5
#define ROOM_PEOPLE_MAX 4

// ���� ���� ������ ���� ����ü
struct SOCKETINFO
{
	int userNo;
	int enterRoomNo;
	WCHAR nickname[NICK_MAX_LEN];
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

extern map<SOCKET, SOCKETINFO*> socketInfoMap;
extern list<ROOMINFO*> roomInfoList;

extern int totalSockets;

void Network();