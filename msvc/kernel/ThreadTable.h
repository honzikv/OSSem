#pragma once
#include <unordered_map>
#include "../api/api.h"

class ThreadTable {

	std::unordered_map<kiv_os::THandle, ThreadControlBlock> table;
};

