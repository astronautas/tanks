#pragma once

#include "Messenger.h"

class Messenger_Client : public Messenger
{
public:
	Messenger_Client();
	~Messenger_Client();

	// Connects to the specified remote address.
	bool Connect(std::string ip_address, unsigned short port);
	void Disconnect();
	void WaitForData();
	virtual bool Send(string text);
	virtual bool Send(int socket, string text);

	bool is_connected = false;
private:
	typedef Messenger super; // note that it could be hidden in
						// protected/private section, instead
};
