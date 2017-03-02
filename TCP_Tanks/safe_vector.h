#pragma once

#include <vector>
#include <mutex>

// Thread safe vector methods
template<typename T>
T * safe_at(std::vector<T> &vector, std::mutex *vector_mutex, int index) {
	lock_guard<mutex> lock(*vector_mutex);

	return &(vector[index]);
};

template<typename T>
void safe_erase(std::vector<T> &vector, std::mutex *vector_mutex, int index) {
	lock_guard<mutex> lock(*vector_mutex);

	vector.erase(vector.begin() + index);
};

template<typename T>
void safe_push(std::vector<T> &vector_to_push, std::mutex *vector_mutex, T value) {
	lock_guard<mutex> lock(*vector_mutex);

	vector_to_push.push_back(value);
};