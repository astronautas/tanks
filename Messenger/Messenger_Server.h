#pragma once

#include "Messenger.h"

class Messenger_Server : public Messenger
{
public:
	Messenger_Server();
	~Messenger_Server();
	
	void Init();
	virtual bool Send(string text);
	virtual bool Send(int socket, string text);
	void Listen();
	void(*on_remove) (int socket); // called when a socket is removed from connections
private:
	bool listening = false;
	typedef Messenger super; // for children to call parent methods
	fd_set active_connections;
};