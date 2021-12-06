#pragma once
#include <iostream>
#include <ostream>
#include <string>
#include <sstream>

// Debug
#define IS_DEBUG false

inline void Log_Debug(const std::string& str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "user_debug:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void Log_Debug(const char* str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "user_debug:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void Log_Warning(const std::string& str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "user_warning:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void Log_Warning(const char* str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "user_warning:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}