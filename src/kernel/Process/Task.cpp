#include "Task.h"

#include "ProcessManager.h"

void Task::SignalSubscribers(const kiv_os::THandle task_id){
	auto lock = std::scoped_lock(task_mutex);
	if (task_state == TaskState::Running) {
		// Projedeme vsechny subscribery a pro kazdeho zavolame callback
		for (const auto& subscriber_handle : on_finish_subscribers) {
			ProcessManager::Get().TriggerSuspendCallback(subscriber_handle, task_id);
		}
		task_state = TaskState::Finished;
	}
}

void Task::AddSubscriber(kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(task_mutex);
	on_finish_subscribers.push_back(subscriber_handle);
	// Navic jeste musime pridat i zaznam pro callback (pokud jeste neni)
	ProcessManager::Get().InitializeSuspendCallback(subscriber_handle);
}

void Task::SetRunning() {
	auto lock = std::scoped_lock(task_mutex);
	if (task_state == TaskState::Ready) {
		task_state = TaskState::Running;
	}
}

void Task::SetBlocked() {
	auto lock = std::scoped_lock(task_mutex);
	if (task_state == TaskState::Finished) {
		return;
	}
	task_state = TaskState::Blocked;
}

void Task::SetFinished() {
	auto lock = std::scoped_lock(task_mutex);
	if (task_state == TaskState::Running) {
		task_state = TaskState::Finished;
	}
}
