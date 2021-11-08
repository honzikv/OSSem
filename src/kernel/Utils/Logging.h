#pragma once
#include <iostream>
#include <string>

#include "Utils/Config.h"

inline void LogDebug(const std::string& str) {
#if IS_DEBUG
	std::cout << "debug:\t" << str << std::endl;
#endif
}

inline void LogDebug(const char* str) {
#if IS_DEBUG
	std::cout << "debug:\t" << str << std::endl;
#endif
}

inline void LogWarning(std::string& str) {
#if IS_DEBUG
	std::cout << "warning:\t" << str << std::endl;
#endif
}

inline void LogWarning(const char* str) {
#if IS_DEBUG
	std::cout << "warning:\t" << str << std::endl;
#endif
}