#pragma once

#include <string>

class Default_Protocol
{
public:
	std::string static Format(std::string message);
private:
	~Default_Protocol();
	Default_Protocol();
};

Default_Protocol::Default_Protocol()
{
}

Default_Protocol::~Default_Protocol()
{
}

std::string Default_Protocol::Format(std::string message) {
	int msg_length = message.length();

	char *formatted_msg = new char[3 + msg_length]{}; // 3 for msg length number chars

	if (msg_length > 256) return false; // disallow msgs longer than 256

	int msg_length_length = to_string(msg_length).length();

	// Add zeroes if msg length number is shorter than 3 symbols
	for (int i = 0; i < 3 - msg_length_length; i++) { formatted_msg[i] = '0'; };

	// Add msg length number to ready to be sent data
	for (int i = 0; i < msg_length_length; i++) { formatted_msg[i + 3 - msg_length_length] = to_string(msg_length)[i]; };

	// Add message to the buffer
	memcpy(&formatted_msg[3], message.c_str(), message.length());

	string final_msg = string(formatted_msg, msg_length + 3);

	return final_msg;
}