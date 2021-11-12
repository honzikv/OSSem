#pragma once
#include <iostream>
#include <string>
#include <sstream>

#include "Config.h"

inline void LogDebug(const std::string& str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "kernel_debug:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void LogDebug(const char* str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "kernel_debug:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void LogWarning(std::string& str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "kernel_warning:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void LogWarning(const char* str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "kernel_warning:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}