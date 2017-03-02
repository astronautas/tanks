#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <thread>
#include "pugi/pugixml.hpp"

#include <chrono>
#include <sstream>

#include <conio.h>

#include <vector>
#include "safe_vector.h"

#include "Messenger_Client.h"
#include "SocketErrorException.h"

#include <mutex>

using namespace std;

struct point {
	int x;
	int y;
};

struct vector_point {
	int x;
	int y;
	int direction;
};

struct tank {
	struct point tank_position;
	int direction; // 0123, rightwise
	int id;
};

struct map {
	int width;
	int height;
	char tiles[20][20] = {};
	char cached_tiles[20][20] = {};
};

string IP;

void init_connect();
void connect_gui(bool first_connection);
void listen();
void render();
void new_msg_callback(int socket, string data);
void run_logic(Messenger_Client * mes);
void update_status();
void move_tank(int direction);
void update();
void update_bullets();

struct point change_coordinates_by_direction(int direction, struct point init_point);

struct map curr_map;
struct tank curr_tank;

vector<struct vector_point> bullets = {};
vector<struct tank> tanks = {};
std::mutex tanks_mutex;

Messenger_Client *mes;

bool joined = false;
bool endgame = false;

pugi::xml_document server_sent_msg_doc;    // character type defaults to char

void init_wsadata() {
	// Init WSADATA
	// otherwise, network f-ality wont work :/
	#ifdef _WIN32
		WSADATA data;
		WSAStartup(MAKEWORD(2, 2), &data);
	#endif
}

bool SkipBOM(std::istream & in)
{
	char test[4] = { 0 };
	in.read(test, 3);
	if (strcmp(test, "\xEF\xBB\xBF") == 0)
		return true;
	in.seekg(0);
	return false;
}

int main(int argc, char *argv[]) {

	init_wsadata();

	mes = new Messenger_Client();
	mes->on_message = new_msg_callback;

	ifstream infile;
	
	////
	string readline;
	char read;
	infile.open("map.txt");
	SkipBOM(infile);
	
	infile >> curr_map.width; infile >> curr_map.height;
	getline(infile, readline);

	for (int i = 0; i < curr_map.width; i++) {
		getline(infile, readline);

		for (int j = 0; j < readline.size(); j++) {
			curr_map.tiles[i][j] = readline[j];
		}
	}

	// Make a map copy to speed up rendering
	for (int i = 0; i < curr_map.width; i++) {

		for (int j = 0; j < curr_map.height; j++) {
			curr_map.cached_tiles[i][j] = curr_map.tiles[i][j];
		}
	}

	///

	connect_gui(true);

	// Run graphics on a separate thread
	thread graphics(render);

	// Listen on a separate thread
	thread logic(run_logic, mes);

	// Join the game
	string msg = "<msg><command>join</command></msg>";

	// Regularly send status update to the game server
	thread update_status_thread(update_status);

	mes->Send(msg);

	graphics.detach();
	logic.detach();
	update_status_thread.detach();

	// Listen for keyboard event in the game
	while (true) {
		if (endgame) {
			cout << "It's all over now...";
			break;
		}


		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

		listen();
		update();
		render();

		std::chrono::system_clock::time_point then = std::chrono::system_clock::now();
		auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::milliseconds(10) - (then - now));

		std::this_thread::sleep_for(difference);
	}

	delete mes;

	return 0;
}

void init_connect() {
	endgame = false;

	try {
		mes->Connect(IP, 53132);
	}
	catch (SocketErrorException ex) {
		cout << ex.what() << endl;
		connect_gui(false);
	}
}

void update() {
	update_bullets();
}

void run_logic(Messenger_Client * mes) {
	try {
		mes->WaitForData();
	}
	catch (SocketErrorException ex) {
		cout << "Disconnected?" << endl;
	}
}

void new_msg_callback(int socket, string data) {

	char* cstr = new char[data.size() + 2];  // Create char buffer to store string copy
	strcpy_s(cstr, _TRUNCATE, data.c_str());             // Copy string into char buffe

	server_sent_msg_doc.load_string(data.c_str());

	string cmd = server_sent_msg_doc.select_single_node("msg").node().select_single_node("command").node().child_value();
	
	if (cmd == "accept_join") {
		string x = server_sent_msg_doc.select_single_node("msg").node().select_single_node("coord").node().attribute("x").value();
		string y = server_sent_msg_doc.select_single_node("msg").node().select_single_node("coord").node().attribute("y").value();

		curr_tank.tank_position.x = stoi(x);
		curr_tank.tank_position.y = stoi(y);

		joined = true;
	}
	else if (cmd == "update_status") {
		if (!joined) return;

		pugi::xml_node tanks_update = server_sent_msg_doc.select_single_node("msg").node().select_single_node("data").node().select_single_node("tanks").node();
		tanks.clear();

		// Iterate tanks
		for (pugi::xml_node tank = tanks_update.select_single_node("tank").node();
			tank; tank = tank = tank.next_sibling())
		{
			struct tank new_tank;
			string x = tank.attribute("x").value();
			new_tank.tank_position.x = stoi(tank.attribute("x").value());
			new_tank.tank_position.y = stoi(tank.attribute("y").value());
			new_tank.id = stoi(tank.attribute("id").value());
			new_tank.direction = 1;

			tanks.push_back(new_tank);
		}
	}
	else if (cmd == "disconnect_destroy") {
		mes->Disconnect();
		endgame = true;
	}
	else if (cmd == "shoot") {
		vector_point new_bullet = {};
		struct point curr_point;
		curr_point.x = stoi(server_sent_msg_doc.select_single_node("msg").node().select_single_node("tank").node().attribute("x").value());
		curr_point.y = stoi(server_sent_msg_doc.select_single_node("msg").node().select_single_node("tank").node().attribute("y").value());
		int direction = stoi(server_sent_msg_doc.select_single_node("msg").node().select_single_node("tank").node().attribute("direction").value());

		struct point new_point = change_coordinates_by_direction(direction, curr_point);
		struct vector_point new_v_point = { new_point.x, new_point.y, direction };
		bullets.push_back(new_v_point);
	}

	delete cstr;
}

void update_status() {
	while (true) {
		if (!joined) continue;

		std::this_thread::sleep_for(std::chrono::milliseconds(40));

		ostringstream stringStream;
		int x = curr_tank.tank_position.x;
		int y = curr_tank.tank_position.y;
		stringStream << "<msg><command>update_status</command><coord x='" << x << "' y='" << y << "'></coord></msg>";

		string msg = stringStream.str();
		mes->Send(msg);
	}
}

// Shoot event
void shoot() {
	int x = curr_tank.tank_position.x;
	int y = curr_tank.tank_position.y;
	int dir = curr_tank.direction;
	ostringstream ss;
	ss << "<msg><command>shoot</command><tank direction='" << dir <<"' x='" << x << "' y='" << y << "'></tank></msg>";

	mes->Send(ss.str());
}

void listen() {
	char key;

	if (_kbhit() != 0) {
		key = _getch();
	}
	else {
		return;
	}

	switch (key)
	{
	case 'w':
		curr_tank.direction = 1;
		move_tank(1);
		render();
		break;
	case 's':
		curr_tank.direction = 3;
		move_tank(3);
		render();
		break;
	case 'a':
		curr_tank.direction = 0;
		move_tank(0);
		render();
		break;
	case 'd':
		curr_tank.direction = 2;
		move_tank(2);
		render();
		break;
	case ' ':
		// Shoot event
		shoot();

		render();
		break;
	}


}

// Direction -> From left clockwise 0123
void move_tank(int direction) {

	// Check if the next tile is not wall
	char next_symbol;
	switch (direction) {
		case 0:
			next_symbol = curr_map.tiles[curr_tank.tank_position.y][curr_tank.tank_position.x - 1];
			if (next_symbol != '|' && next_symbol != '-') { 
				curr_tank.tank_position.x -= 1; 
			}
			break;
		case 1:
			next_symbol = curr_map.tiles[curr_tank.tank_position.y - 1][curr_tank.tank_position.x];
			if (next_symbol != '|' && next_symbol != '-') { curr_tank.tank_position.y -= 1; }
			break;
		case 2:
			next_symbol = curr_map.tiles[curr_tank.tank_position.y][curr_tank.tank_position.x + 1];
			if (next_symbol != '|' && next_symbol != '-') { curr_tank.tank_position.x += 1; }
			break;
		case 3:
			next_symbol = curr_map.tiles[curr_tank.tank_position.y + 1][curr_tank.tank_position.x];
			if (next_symbol != '|' && next_symbol != '-') { curr_tank.tank_position.y += 1; }
			break;
	}
}

// GUI
void connect_gui(bool first_connection) {
	cout << "Enter IP address: ";
	cin >> IP;

	if (first_connection) cout << "Welcome to the tanks game. It's gonna be great. Huge. Believe me." << endl;

	cout << "Press ENTER to connect..." << endl;

	cin.get();

	init_connect();
}

void render() {
	// Clear the map
	system("cls");

	// Clear map tanks, bullets
	for (int i = 0; i < curr_map.width; i++) {

		for (int j = 0; j < curr_map.height; j++) {
			curr_map.tiles[i][j] = curr_map.cached_tiles[i][j];
		}
	}

	// Display the player tank
	char pos_arrow;

	switch (curr_tank.direction) {
		case 0:
			pos_arrow = '<';
			break;
		case 1:
			pos_arrow = '^';
			break;
		case 2:
			pos_arrow = '>';
			break;
		case 3:
			pos_arrow = 'v';
			break;
	}

	curr_map.tiles[curr_tank.tank_position.y][curr_tank.tank_position.x] = pos_arrow;

	// Add other players (if any existing)
	for (int i = 0; i < tanks.size(); i++) {
		curr_map.tiles[tanks[i].tank_position.y][tanks[i].tank_position.x] = '&';
	}

	// Render bullets
	for (int i = 0; i < bullets.size(); i++) {
		curr_map.tiles[bullets[i].y][bullets[i].x] = '*';
	}

	// Show the map
	for (int i = 0; i < curr_map.width; i++) {

		for (int j = 0; j < curr_map.height; j++) {
			cout << curr_map.tiles[i][j];
		}

		cout << "\n";
	}
}

void update_bullets() {

	for (int i = 0; i < bullets.size(); i++) {
		bool bullet_alive = true;

		switch (bullets[i].direction) {
			case 0:
				bullets[i].x--;
				break;
			case 1:
				bullets[i].y--;
				break;
			case 2:
				bullets[i].x++;
				break;
			case 3:
				bullets[i].y++;
				break;
		}

		// Check if the current bullet hit the tank
		for (int j = 0; j < tanks.size(); j++) {
			int id = safe_at(tanks, &tanks_mutex, j)->id;
			struct point tank_pos = safe_at(tanks, &tanks_mutex, j)->tank_position;

			if (tank_pos.x == bullets[i].x && tank_pos.y == bullets[i].y) {
				//safe_erase(tanks, &tanks_mutex, j);

				ostringstream ss;
				ss << "<msg><command>destroy_tank</command><tank id='" << id << "'></tank></msg>";

				string msg = ss.str();
				mes->Send(msg);

				// Erase the bullet
				bullets.erase(bullets.begin() + i);
				bullet_alive = false;
			}
		}

		if (bullet_alive) {
			if (bullets[i].x == 0 || bullets[i].y == 0 || bullets[i].x == curr_map.width - 2
				|| bullets[i].y == curr_map.height - 1 || curr_map.tiles[bullets[i].y][bullets[i].x] == '|'
				|| curr_map.tiles[bullets[i].y][bullets[i].x] == '-') {

				bullets.erase(bullets.begin() + i);
			}
		}
	}
}

struct point change_coordinates_by_direction(int direction, struct point init_point) {

	switch (direction) {
		case 0:
			init_point.x--;
			break;
		case 1:
			init_point.y--;
			break;
		case 2:
			init_point.x++;
			break;
		case 3:
			init_point.y++;
			break;
	}

	return init_point;
}