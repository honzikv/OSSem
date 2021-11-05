#include "ProcessManager.h"



kiv_os::THandle ProcessManager::getFreePid() {
	std::scoped_lock(processTableMutex);
	for (uint16_t i = 0; i < static_cast<uint16_t>(processTable.size()); i += 1) {
		if (processTable[i] == nullptr) {
			return i + PID_RANGE_START;
		}
	}

	return NO_FREE_ID;
}

kiv_os::THandle ProcessManager::getFreeTid() {
	std::scoped_lock(threadTableMutex);
	for (uint16_t i = 0; i < static_cast<uint16_t>(threadTable.size()); i += 1) {
		if (threadTable[i] == nullptr) {
			return i + TID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

std::shared_ptr<Process> ProcessManager::getProcess(const kiv_os::THandle pid) {
	return processTable[pid + PID_RANGE_START];
}
