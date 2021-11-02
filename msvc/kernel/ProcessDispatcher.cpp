#include "ProcessDispatcher.h"
kiv_os::THandle ProcessDispatcher::getFreePid() {
	for (uint16_t i = 0; i < static_cast<uint16_t>(processTable.size()); i += 1) {
		if (processTable[i] == nullptr) {
			return i + PID_RANGE_START;
		}
	}

	return NO_FREE_ID;
}

kiv_os::THandle ProcessDispatcher::getFreeTid() {
	for (uint16_t i = 0; i < static_cast<uint16_t>(threadTable.size()); i += 1) {
		if (threadTable[i] == nullptr) {
			return i + TID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}
