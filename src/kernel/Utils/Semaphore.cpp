#include "Semaphore.h"
void Semaphore::acquire() {
	std::unique_lock lock(mutex);
	while (count == 0) {
		conditionVariable.wait(lock);
	}
	count -= 1;
}

void Semaphore::release() {
	std::scoped_lock lock(mutex);
	count += 1;
	conditionVariable.notify_one();
}
