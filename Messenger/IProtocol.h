#pragma once

#include <string>

class IProtocol
{
public:
	IProtocol();
	~IProtocol();

	virtual static std::string Format(std::string message) = 0;
private:

};

IProtocol::IProtocol()
{
}

IProtocol::~IProtocol()
{
}