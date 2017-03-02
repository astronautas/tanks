#pragma once
#include <vector>
#include <mutex>

// Thread safe vector methods
template<typename T>
T * safe_at(std::vector<T> &vector, int index) {
	lock_guard<mutex> lock(clients_mutex);

	return &(vector[index]);
};

template<typename T>
void safe_erase(std::vector<T> &vector, int index) {
	lock_guard<mutex> lock(clients_mutex);

	vector.erase(vector.begin() + index);
};

template<typename T>
void safe_push(std::vector<T> &vector_to_push, T value) {
	lock_guard<mutex> lock(clients_mutex);

	vector_to_push.push_back(value);
};