#include "Messenger.h"
#include "Messenger_Server.h"
#include "Config.h"
#include "SocketErrorException.h"

#include <vector>
#include "safe_vector.h"
#include <iostream>

#include "rapid_xml/rapidxml.hpp"
#include "rapid_xml/rapidxml_print.hpp"

#include <sstream>

#include <thread>
#include <mutex>

#include <iterator>     // std::back_inserter

void callback(int socket, string data);
void remove_callback(int socket);
void update_all();

using namespace rapidxml;

Messenger_Server *server;

struct tank {
	int x;
	int y;
	int direction;
};

struct client {
	int socket;
	struct tank tank;
	int id;
};

struct point {
	int x;
	int y;
};

vector<struct client> clients = {};
std::mutex clients_mutex;  // protects g_i

int main(int argc, char *argv[]) {

	// Init WSADATA
	// otherwise, network functionality wont work :/
	#ifdef _WIN32
		WSADATA data;
		WSAStartup(MAKEWORD(2, 2), &data);
	#endif

	server = new Messenger_Server();
	server->Init();

	server->on_message = callback;
	server->on_remove = remove_callback;

	// Start running regular status updates
	thread updates(update_all);

	try {
		server->Listen();
	}
	catch (SocketErrorException) {
		cout << "Socket error exception" << endl;
		exit(1);
	}

	delete server;
	updates.join();
}

void remove_callback(int socket) {

	for (int i = 0; i < clients.size(); i++) {
		
		if (safe_at(clients, i)->socket == socket) {
			safe_erase(clients, i);

			cout << "Disconnected client: " << socket << endl;
		}
	}
}

void callback(int socket, string data) {
	xml_document<> doc;    // character type defaults to char

	char* cstr = new char[data.size() + 2];  // Create char buffer to store string copy
	strcpy_s(cstr, _TRUNCATE, data.c_str());             // Copy string into char buffer

	doc.parse<0>(cstr);

	string cmd = doc.first_node()->first_node("command")->value();

	// Small router (temp)
	if (cmd == "join") {
		client new_client;

		new_client.socket = socket;

		new_client.tank.x = 1;
		new_client.tank.y = 1;
		new_client.tank.direction = 1;

		new_client.id = rand() % 1000;

		safe_push(clients, new_client);

		server->Send(socket, "<msg><command>accept_join</command><coord x='1' y='1'></coord></msg>");

		cout << "New socket: " << socket << endl;
	}
	else if (cmd == "update_status") {
		int x = stoi(doc.first_node()->first_node("coord")->first_attribute("x")->value());
		int y = stoi(doc.first_node()->first_node("coord")->first_attribute("y")->value());

		// Find client
		for (int i = 0; i < clients.size(); i++) { 

			if (safe_at(clients, i)->socket == socket) {
				safe_at(clients, i)->tank.x = x;
				safe_at(clients, i)->tank.y = y;

				safe_at(clients, i)->tank.direction = 1;

				break;
			}
		}
	}
	else if (cmd == "destroy_tank") {
		int id = stoi(doc.first_node()->first_node("tank")->first_attribute("id")->value());

		// Inform destroyed client.
		// No need to remove it - when the client disconnects, it will be autoremoved by the messenger
		string msg = "<msg><command>disconnect_destroy</command></msg>";

		for (int i = 0; i < clients.size(); i++) {
			struct client *curr_client = safe_at(clients, i);

			if (safe_at(clients, i)->id == id) {
				server->Send(curr_client->socket, msg);
			}
		}
	}
	else if (cmd == "shoot") {
		// Redirect message to all clients
		for (int i = 0; i < clients.size(); i++) {
			cout << "Someone shot!" << endl;
			server->Send(safe_at(clients, i)->socket, data);
		}
	}

	delete cstr;
}

void update_all() {

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_CLIENT_UPDATE_DELAY));

		for (int i = 0; i < clients.size(); i++) {
			ostringstream stringStream;
			int x = safe_at(clients, i)->tank.x;
			int y = safe_at(clients, i)->tank.y;
			stringStream << "<msg><command>update_status</command></msg>";
			xml_document<> doc;

			xml_node<> *msg_node = doc.allocate_node(node_element, doc.allocate_string("msg"));
			xml_node<> *cmd_node = doc.allocate_node(node_element, doc.allocate_string("command"));
			xml_node<> *data_node = doc.allocate_node(node_element, doc.allocate_string("data"));
			doc.append_node(msg_node);
			cmd_node->value("update_status");
			msg_node->append_node(cmd_node);
			msg_node->append_node(data_node);

			char *tanks_node_name = doc.allocate_string("tanks");        // Allocate string and copy name into it
			xml_node<> *tanks = doc.allocate_node(node_element, tanks_node_name);  // Set node name to node_name
			data_node->append_node(tanks);

			vector<const char *> generated_attrs = {};

			for (int j = 0; j < clients.size(); j++) {

				// If current tank is being sent to its owner, mark it
				if (safe_at(clients, i)->socket == safe_at(clients, j)->socket) {
					continue;
				}

				char *tank_node_name = doc.allocate_string("tank");        // Allocate string and copy name into it
				xml_node<> *tank = doc.allocate_node(node_element, tank_node_name);  // Set node name to node_name

				string x_str = static_cast<ostringstream*>(&(ostringstream() << safe_at(clients, j)->tank.x))->str();
				string y_str = static_cast<ostringstream*>(&(ostringstream() << safe_at(clients, j)->tank.y))->str();
				string direction_str = static_cast<ostringstream*>(&(ostringstream() << safe_at(clients, j)->tank.direction))->str();
				string id_str = static_cast<ostringstream*>(&(ostringstream() << safe_at(clients, j)->id))->str();

				char *x_char = new char[x_str.size() + 1]; x_char[x_str.size()] = '\0';
				char *y_char = new char[y_str.size() + 1]; y_char[y_str.size()] = '\0';
				char *direction_char = new char[direction_str.size() + 1]; direction_char[direction_str.size()] = '\0';
				char *id_char = new char[id_str.size() + 1]; id_char[id_str.size()] = '\0';

				// To later save memory while deleting them
				generated_attrs.push_back(x_char);
				generated_attrs.push_back(y_char);
				generated_attrs.push_back(direction_char);
				generated_attrs.push_back(id_char);

				strcpy_s(x_char, x_str.length() + 1, x_str.c_str()); // to, to length, from
				strcpy_s(y_char, y_str.length() + 1, y_str.c_str());
				strcpy_s(direction_char, direction_str.length() + 1, direction_str.c_str());
				strcpy_s(id_char, id_str.length() + 1, id_str.c_str());

				xml_attribute<> *x = doc.allocate_attribute("x", x_char);
				xml_attribute<> *y = doc.allocate_attribute("y", y_char);
				xml_attribute<> *direction = doc.allocate_attribute("direction", direction_char);
				xml_attribute<> *id = doc.allocate_attribute("id", id_char);

				tank->append_attribute(x); tank->append_attribute(y); tank->append_attribute(id);

				tanks->append_node(tank);
			}

			// Print to string using output iterator
			string xml_as_string;

			// watch for name collisions here, print() is a very common function name!
			print(std::back_inserter(xml_as_string), doc);

			// Free up memory
			for (int i = 0; i < generated_attrs.size(); i++) { delete generated_attrs[i]; }

			if (clients.size() > 0) {

				server->Send(safe_at(clients, i)->socket, xml_as_string);
			}
		}
	}


}


