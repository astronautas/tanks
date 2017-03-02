#include "Messenger_Client.h"

// Explain each
#ifdef _WIN32
		#include <WinSock2.h>
		#include <WS2tcpip.h>
		#include <Windows.h>
#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

using namespace std;

Messenger_Client::Messenger_Client()
{
}

Messenger_Client::~Messenger_Client()
{
}

bool Messenger_Client::Connect(string ip_address, unsigned short port) {

	if ((port < 1) || (port > 65535)) {
		throw SocketErrorException("The Port value is not in the range of 0-65535!");
	}

	curr_socket = socket(AF_INET, SOCK_STREAM, 0);

	// Unless socket is succesfully created, return false
	if (curr_socket < 0) {
		int error = errno;

		// TODO: Throw exception
		throw SocketErrorException("Cannot create socket with specified data.");
	}

	const char *ip_chars = ip_address.c_str();

	server_struct.sin_family = AF_INET; // The protocol is IP
	server_struct.sin_port = htons(port); // u_short -> tcp/ip network byte order
	inet_pton(AF_INET, ip_chars, &server_struct.sin_addr.s_addr);

	// Need to cast server_struct to sockaddr from sockaddr_in
	int connected = connect(curr_socket, (const sockaddr*) &server_struct, sizeof(server_struct));

	if (connected < 0) {
		int error = errno;

		// TODO: Throw exception
		throw SocketErrorException("Cannot connect to specified remote (offline).");
	}

	is_connected = true;

	return true;
}

void Messenger_Client::Disconnect() {

	if (is_connected) {
		is_connected = false;
		Close_socket(curr_socket);
	}
}

bool Messenger_Client::Send(string text) {
	return Send(curr_socket, text);
}

bool Messenger_Client::Send(int socket, string text) {
	return super::Send(socket, text);
}

void Messenger_Client::WaitForData() {
	int fdmax = curr_socket;
	fd_set readfds = {};
	fd_set masterfds = {};

	FD_SET(curr_socket, &masterfds);

	while (true) {
		if (!is_connected) return;

		//clear the socket set
		FD_ZERO(&readfds);
		readfds = masterfds;

		// Hangs until any of sockets are ready for reading (have new data)
		if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1) {
			throw SocketErrorException("A select error has occured.");
		}

		// Check if server socket is ready for reading
		if (FD_ISSET(curr_socket, &readfds)) {

			// Reading client data
			int readbytes;

			// First 3 symbols indicate msg length
			char len_buff[4]; len_buff[3] = '\0';
			recv(curr_socket, len_buff, 3, 0);
			int len = 0;
			sscanf_s(len_buff, "%d", &len); // len char array to integer

			char *msg_buffer = new char[len];
			readbytes = recv(curr_socket, msg_buffer, len, 0);

			// Client sends 0 bytes on orderly disconnect
			if (readbytes <= 0) {
				Close_socket(curr_socket);

				FD_CLR(curr_socket, &masterfds); // remove from master set
			}
			else {
				on_message(curr_socket, string(msg_buffer, len));
			}

			delete msg_buffer;
		}


	}
}