#include "Task.h"

#include "ProcessManager.h"

const std::atomic<TaskState>& Task::Get_Task_State() const { return task_state; }

const std::vector<kiv_os::THandle>& Task::Get_On_Finish_Subscribers() const { return on_finish_subscribers; }

uint16_t Task::Get_Task_Exit_Code() const { return task_exit_code; }

void Task::Add_Subscriber(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(subscribers_mutex);
	// Cekat muzeme pouze pokud se task jiz nevykonal
	if (task_state != TaskState::Finished) {
		on_finish_subscribers.push_back(subscriber_handle);
		// Navic jeste musime pridat i zaznam pro callback (pokud jeste neni)
		ProcessManager::Get().Initialize_Suspend_Callback(subscriber_handle);
	}
}

void Task::Notify_Subscribers(const kiv_os::THandle this_task_handle) {
	auto lock = std::scoped_lock(subscribers_mutex);
	for (const auto& subscriber_handle : on_finish_subscribers) {
		ProcessManager::Get().Trigger_Suspend_Callback(subscriber_handle, this_task_handle);
	}
}

void Task::Set_Exit_Code(const uint16_t exit_code) {
	task_exit_code = exit_code;
}

void Task::Set_Running() {
	if (task_state == TaskState::Ready) {
		task_state = TaskState::Running;
	}
}

void Task::Set_Finished() {
	task_state = TaskState::Finished;
}
