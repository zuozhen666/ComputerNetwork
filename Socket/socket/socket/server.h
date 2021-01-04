#pragma once
#include<WinSock2.h>
#include<fstream>
#include<iostream>
#include<WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SIZE 1024*8
#define IP_ADDR "127.0.0.1"
#define Port 8080

using namespace std;

void HandleRequest(SOCKET cliSocket, char* recvdata);
void Send404Info(SOCKET cliSocket);