#pragma once
#include <iostream>
#include <string>
#include <sstream>

#include "Debug.h"

/// Jednoduche funkce pro logovani. Pokud je debug vypnuty, funkce nic nedelaji (resp. compiler by ani nemel funkce vytvorit)

inline void Log_Debug(const std::string& str) {
#if IS_DEBUG
	auto string_stream = std::stringstream();
	string_stream << "kernel_debug:\t" << str << std::endl;
	std::cout << string_stream.str();
#endif
}

inline void Log_Debug(const char* str) {
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