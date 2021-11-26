#include "Thread.h"
#include "ProcessManager.h"
#include <csignal>

void Thread::ThreadFunc() {
	// Vlakno se spustilo - tzn nastavime stav vlakna na bezici
	Set_Running();

	// Provedeme spusteni programu (funkce TThread_Proc), coz vlakno zablokuje dokud nedobehne
	task_exit_code = static_cast<uint16_t>(program(regs));
		
	auto lock = std::scoped_lock(thread_finish_mutex);
	if (task_state != TaskState::Running) {
		// Pokud jiz proces nebezi, nemuzeme zavolat on thread finish a pravdepodobne se ukoncil nasilim
		return;
	}
	ProcessManager::Get().On_Thread_Finish(tid);
	task_state = TaskState::Finished;
}

Thread::Thread(const kiv_os::TThread_Proc program, const kiv_hal::TRegisters context, const kiv_os::THandle tid,
               const kiv_os::THandle pid,
               const char* args, const bool is_main_thread): program(program), regs(context),
                                                             main_thread(is_main_thread),
                                                             args(args),
                                                             tid(tid), pid(pid) {
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(this->args.c_str());
}


std::thread::id Thread::Dispatch() {
	auto thread = std::thread(&Thread::ThreadFunc, this);
	const auto thread_id = thread.get_id();
	thread.detach();
	return thread_id;
}

void Thread::Terminate_If_Running(const uint16_t exit_code) {
	auto lock = std::scoped_lock(thread_finish_mutex); // Locknutim mutexu vime, ze muzeme vlakno vypnout
	if (task_state == TaskState::Running) {
		// Nastavime exit code a task state jako program terminated
		task_exit_code = exit_code;
		task_state = TaskState::Finished;
	}
}

TaskState Thread::GetState() const { return task_state; }
