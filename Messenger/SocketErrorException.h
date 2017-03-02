#pragma once

#include <exception>

using namespace std;

class SocketErrorException : public exception {
private:
	const char *msg = "Socket error exception.";

public:
	SocketErrorException() {
	}

	SocketErrorException(const char *m) {
		msg = m;
	}

	virtual const char* what() const throw()
	{
		return msg;
	}
};