#include "SuspendCallback.h"

#include "Utils/Logging.h"

void SuspendCallback::Suspend() const {
	semaphore->Acquire();
}

void SuspendCallback::Notify(const kiv_os::THandle notifier_id) {
	if (triggered) { return; } // toto muzeme udelat i bez locku, protoze je "triggered" atomic
	auto lock = std::scoped_lock(mutex);
	this->notifier_id = notifier_id;
	triggered = true;
	semaphore->Release();
}

bool SuspendCallback::Triggered() {
	return triggered;
}

kiv_os::THandle SuspendCallback::GetNotifierId() const {
	return notifier_id;
}
