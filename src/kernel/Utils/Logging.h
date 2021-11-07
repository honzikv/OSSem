#pragma once
#include <iostream>
#include <string>

#include "Utils/Config.h"

inline void Log(std::string& str) {
#if IS_DEBUG
	std::cout << "debug:\t" << str << std::endl;
#endif
}

inline void Log(const char* str) {
#if IS_DEBUG
	std::cout << "debug:\t" << str << std::endl;
#endif
}