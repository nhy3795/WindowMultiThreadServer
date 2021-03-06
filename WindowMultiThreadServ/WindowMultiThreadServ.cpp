// WindowMultiThreadServ.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <iostream>



unsigned WINAPI HandleClnt(void * arg);
unsigned WINAPI QuitThread(void *);
void SendMsg(std::string name, SOCKET mySocket);

int clntCnt = 0;
std::vector<SOCKET> clntSocks;
HANDLE hMutex, hPrintMutex;

struct ThreadData
{
	SOCKET socket;
	std::string ip;
};

int main(int argc, char * argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrSz;
	HANDLE hThread;
	if (argc != 2)
	{
		printf("%s <port>", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DebugBreak();
	}

	hMutex = CreateMutex(NULL, FALSE, NULL);
	hPrintMutex = CreateMutex(NULL, FALSE, NULL);
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (sockaddr *)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		DebugBreak();
	}
	if (listen(hServSock, 5) == SOCKET_ERROR)
	{
		DebugBreak();
	}

	_beginthreadex(NULL, 0, QuitThread, nullptr, 0, NULL);

	while (1)
	{

		clntAdrSz = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR *)&clntAdr, &clntAdrSz);
		WaitForSingleObject(hMutex, INFINITE);
		clntSocks.push_back(hClntSock);
		ReleaseMutex(hMutex);

		ThreadData * data = new ThreadData();

		data->ip = inet_ntoa(clntAdr.sin_addr);
		data->socket = hClntSock;
		std::cout << "Connected Client IP : " << data->ip << std::endl;

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, data, 0, NULL);
	}
	closesocket(hServSock);
	WSACleanup();

	return 0;
}

unsigned WINAPI HandleClnt(void * arg)
{
	ThreadData * data = (ThreadData *)arg;
	int strLen = 0;
	char msg[256 * 2];		
	memset(&msg, 0, sizeof(msg));
	while (((strLen = recv(data->socket, msg, sizeof(msg), 0)) >= 0))
	{
		if (strLen < sizeof(msg) - 1)
		{
			SendMsg(msg, data->socket);
		}
		memset(&msg, 0, sizeof(msg));
	}
	WaitForSingleObject(hPrintMutex, INFINITE);
	if (strLen == -1)
	{
		SendMsg("Disconnected Client : " + data->ip + "\n", data->socket);

	}
	ReleaseMutex(hPrintMutex);

	WaitForSingleObject(hMutex, INFINITE);
	auto iter = std::find(clntSocks.begin(), clntSocks.end(), data->socket);
	if (iter != clntSocks.end())
	{
		clntSocks.erase(iter);
	}
	ReleaseMutex(hMutex);
	closesocket(data->socket);
	delete data;

	return 0;
}

unsigned WINAPI QuitThread(void *)
{ 
	char c = ' '; 
	while (c != 'q')
	{
		std::cin >> c;
	}

	WaitForSingleObject(hMutex, INFINITE);
	WaitForSingleObject(hPrintMutex, INFINITE);
	exit(0);
	ReleaseMutex(hPrintMutex);
	ReleaseMutex(hMutex);

	return 0;
}

void SendMsg(std::string string, SOCKET mySocket)
{
	WaitForSingleObject(hMutex, INFINITE);
	std::cout << string;
	for (auto & socket : clntSocks)
	{
		if (socket != mySocket)
		{
			send(socket, string.c_str(), string.length(), 0);
		}
	}
	ReleaseMutex(hMutex);
}
