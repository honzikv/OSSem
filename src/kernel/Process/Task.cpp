#include "Task.h"

#include "ProcessManager.h"

void Task::NotifySubscribers(const kiv_os::THandle task_id, const bool terminated_forcefully){
	auto lock = std::scoped_lock(mutex);
	if (task_state == TaskState::Running) {
		// Projedeme vsechny subscribery a pro kazdeho zavolame callback
		for (const auto& subscriber_handle : on_finish_subscribers) {
			ProcessManager::Get().TriggerSuspendCallback(subscriber_handle, task_id);
		}

		// Pokud se tato metoda zavolala primo z vlakna / procesu nastavime jeho stav jako ukonceny,
		// jinak bude stav jako Terminated aby neslo subscribery signalizovat vicekrat
		task_state = terminated_forcefully ? TaskState::Terminated : TaskState::Finished;
	}
}

void Task::AddSubscriber(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(mutex);
	on_finish_subscribers.push_back(subscriber_handle);
	// Navic jeste musime pridat i zaznam pro callback (pokud jeste neni)
	ProcessManager::Get().InitializeSuspendCallback(subscriber_handle);
}

void Task::SetExitCode(const uint16_t exit_code) {
	auto lock = std::scoped_lock(mutex);
	task_exit_code = exit_code;
}

void Task::SetRunning() {
	auto lock = std::scoped_lock(mutex);
	if (task_state == TaskState::Ready) {
		task_state = TaskState::Running;
	}
}

void Task::SetFinished() {
	auto lock = std::scoped_lock(mutex);
	if (task_state == TaskState::Running) {
		task_state = TaskState::Finished;
	}
}

uint16_t Task::GetExitCode() {
	auto lock = std::scoped_lock(mutex);
	if (task_state == TaskState::Finished) {
		return task_exit_code;
	}

	return -1;
}
