#include "Init.h"

void InitProcess::NotifySubscribers(kiv_os::THandle task_id, bool called_from_task) {
	semaphore->Release();
}

void InitProcess::Dispatch() {
	ProcessManager::Get().RunInitProcess(InitFun);
	semaphore->Acquire();
}
