#include "Semaphore.h"

#include "Logging.h"

void Semaphore::Acquire() {
	std::unique_lock lock(mutex);
	while (count == 0) {
		condition_variable.wait(lock);
	}
	count -= 1;
	Log("Semaphore acquired");
}

void Semaphore::Release() {
	std::scoped_lock lock(mutex);
	Log("Releasing lock");
	count += 1;
	condition_variable.notify_one();
}
