#include "Thread.h"
#include "ProcessManager.h"


void Thread::Thread_Func() {
	Set_Running();
	task_exit_code = static_cast<kiv_os::NOS_Error>(program(regs)); // ziskame exit code z programu

	// Program dobehl, takze muzeme notifikovat vsechny subscribery
	Finish(tid);

	// Pokud je toto vlakno main, ukoncime i proces
	if (is_main_thread) {
		ProcessManager::Get().Finish_Process(pid);
	}
	
}

std::thread::id Thread::Dispatch() {
	auto thread = std::thread(&Thread::Thread_Func, this);
	const auto thread_id = thread.get_id();
	thread.detach(); // vlakno musime detachnout
	return thread_id; // vratime thread id
}

TaskState Thread::Get_State() { return task_state; }
