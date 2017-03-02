#include "Messenger_Server.h"
#include "Config.h"
#include <iostream>

Messenger_Server::Messenger_Server()
{

}

Messenger_Server::~Messenger_Server()
{
}

void Messenger_Server::Init()
{
	// Converts a u_long from host to TCP/IP network byte order (which is big-endian).
	server_struct.sin_addr.s_addr = htonl(SERVER_IP);
	server_struct.sin_port = htons(SERVER_PORT); // u_short -> tcp/ip network byte order
	server_struct.sin_family = SERVER_PROTOCOL_FAMILY; // The protocol is IP
}

bool Messenger_Server::Send(string text) {
	return Send(curr_socket, text);
}

bool Messenger_Server::Send(int socket, string text) {

	if (super::Send(socket, text) == false) {
		Close_socket(socket);
		on_remove(socket);

		return false;
	}

	return true;
}

// Listen for incomming connections
void Messenger_Server::Listen() {
	struct sockaddr_storage remoteaddr; // client address

	int newfd;

	fd_set read_fds = {};  // temp file descriptor list for select()

	int fdmax;

	int listener = socket(AF_INET, SOCK_STREAM, 0);

	int test;

	if (test = bind(listener, (struct sockaddr *)&server_struct, sizeof(server_struct)) < 0) {
		int error_code = errno;

		throw SocketErrorException("Error occured when trying to bind the address with a socket.");
	}

	if (listen(listener, 10) < 0) {
		int error_code = errno;

		throw SocketErrorException("Error occured when trying to start listening.");
	}

	cout << "Listening on " << SERVER_IP << ":" << SERVER_PORT << endl;

	FD_SET(listener, &active_connections); // add listener socket to master list

	fdmax = listener; // track biggest socket (by number). Currently, it's listener

	// Main listening loop
	while (true) {
		read_fds = active_connections;

		// Checks the number of sockets in sets meeting conditions.
		// Last param NULL - wait infinitelly (a.k.a. waiting for connections)
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			int error_code = errno;

			throw SocketErrorException("A select error has occured.");
		}

		// Run through connections (for new data)
		// When select finds any of readable fd in readfs, it replaces read_fds with 'em
		// Gotta check then which ones were left as readable
		for (int i = 0; i <= fdmax; i++) {

			// Check if current contains data ready to be read
			if (FD_ISSET(i, &read_fds)) {

				// Check listener for new connections
				if (i == listener) {

					// Get address length
					int addrlen = sizeof remoteaddr;

					// Accept the new connection
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

					if (newfd != -1) {
						FD_SET(newfd, &active_connections); // add the new connection to connection list

						if (newfd > fdmax) {
							fdmax = newfd; // new max
						}
					}
					else {
						int error_code = errno;

						throw SocketErrorException("A connection accept error has occured.");
					}
				} else {

					// Reading client data
					int readbytes;

					// First 3 symbols indicate msg length
					char len_buff[4]; len_buff[3] = '\0';
					recv(i, len_buff, 3, 0);
					int len = 0;
					sscanf_s(len_buff, "%d", &len); // len char array to integer

					char *msg_buffer = new char[len];
					readbytes = recv(i, msg_buffer, len, 0);

					// Negative read byte count indicates a closed socket
					if (readbytes <= 0) {
						on_remove(i); // broadcast that a socket was removed from connections

						Close_socket(i);
						FD_CLR(i, &active_connections); // remove from active connection set
					}
					else {
						on_message(i, string(msg_buffer, len)); // pass the message to the handler function
					}

					delete msg_buffer;
				}
			}
		}
	}
}



