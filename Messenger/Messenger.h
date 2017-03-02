#pragma once

#include <string>
#include "SocketErrorException.h"

#ifdef _WIN32
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <Windows.h>
	#define socklen_t int
#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h>
#endif

using namespace std;

class Messenger
{
public:
	Messenger();
	~Messenger();
	
	virtual bool Send(string text);
	virtual bool Send(int socket, string text);
	void(*on_message) (int socket, string data);
protected:
	int curr_socket;
	struct sockaddr_in server_struct;

	string(*Format)(string message);
	void Close_socket(int socket);
};

