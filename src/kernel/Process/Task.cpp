#include "Task.h"

#include "ProcessManager.h"

const std::atomic<TaskState>& Task::GetTaskState() const { return task_state; }

const std::vector<kiv_os::THandle>& Task::GetOnFinishSubscribers() const { return on_finish_subscribers; }

uint16_t Task::GetTaskExitCode() const { return task_exit_code; }

void Task::AddSubscriber(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(subscribers_mutex);
	// Cekat muzeme pouze pokud se task jiz nevykonal
	if (task_state != TaskState::ProgramFinished && task_state != TaskState::ProgramTerminated) {
		on_finish_subscribers.push_back(subscriber_handle);
		// Navic jeste musime pridat i zaznam pro callback (pokud jeste neni)
		ProcessManager::Get().InitializeSuspendCallback(subscriber_handle);
	}
}

void Task::NotifySubscribers(const kiv_os::THandle this_task_handle) {
	auto lock = std::scoped_lock(subscribers_mutex);
	for (const auto& subscriber_handle : on_finish_subscribers) {
		ProcessManager::Get().TriggerSuspendCallback(subscriber_handle, this_task_handle);
	}
}

void Task::SetExitCode(const uint16_t exit_code) {
	task_exit_code = exit_code;
}

void Task::SetRunning() {
	if (task_state == TaskState::Ready) {
		task_state = TaskState::Running;
	}
}

void Task::SetReadableExitCode() {
	task_state = TaskState::ReadableExitCode;
}
