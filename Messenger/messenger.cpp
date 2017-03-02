#include "Messenger.h"
#include "Default.h"
#include "Config.h"

#ifdef _WIN32
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <Windows.h>

	// Need to link with Ws2_32.lib
	#pragma comment(lib, "Ws2_32.lib")
#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

using namespace std;

Messenger::Messenger()
{
	Format = MESSAGE_PROTOCOL::Format;
}

Messenger::~Messenger()
{
}

// When socket is not specified, 
// curr_socket is used (i.e. on client side, the server socket)
bool Messenger::Send(string text)
{
	return Send(curr_socket, text);
}

bool Messenger::Send(int socket, string text) {
	
	string formatted_msg = Format(text);
	char *formatted_msg_chars = new char[formatted_msg.length()];

	// Add message to the buffer
	memcpy(formatted_msg_chars, formatted_msg.c_str(), formatted_msg.length());

	if (send(socket, formatted_msg_chars, 3 + text.length(), 0) < 0) {
		return false;
	}

	delete formatted_msg_chars;

	return true;
}

// close is for UNIX
void Messenger::Close_socket(int socket)
{
	#ifdef _WIN32
		closesocket(socket);
	#else
		close(socket);
	#endif
}