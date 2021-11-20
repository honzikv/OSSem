#include "SuspendCallback.h"

#include "Utils/Logging.h"

void SuspendCallback::Suspend() const {
	semaphore->Acquire();
}

void SuspendCallback::Notify(const kiv_os::THandle notifier_id) {
	auto lock = std::scoped_lock(mutex);
	if (triggered) { return; }
	this->notifier_id = notifier_id;
	triggered = true;
	semaphore->Release();
}


kiv_os::THandle SuspendCallback::GetNotifierId() const {
	return notifier_id;
}
