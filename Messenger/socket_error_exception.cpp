#include <exception>
#include "SocketErrorException.h"

using namespace std;

class SocketErrorException : public exception {

	virtual const char* what() const throw()
	{
		return "Socket error exception.";
	}
} SocketErrorException;