#include "Semaphore.h"

#include "Logging.h"

void Semaphore::Acquire() {
	auto lock = std::unique_lock(mutex);
	while (count == 0) {
		condition_variable.wait(lock);
	}
	count -= 1;
}

void Semaphore::Release() {
	auto lock = std::scoped_lock(mutex);
	count += 1;
	condition_variable.notify_one();
}

Semaphore::Semaphore(const size_t count): count(count) {}
