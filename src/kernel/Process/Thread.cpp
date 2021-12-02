#include "Thread.h"
#include "ProcessManager.h"
#include <csignal>

void Thread_Func(const std::shared_ptr<Thread> thread) {
	// Tato funkce musi kopirovat shared ptr, jinak se pri shutdownu muze stat, ze se shared_ptr smaze
	// a na data uz nebudeme mit referenci. Shared pointerem zarucime, ze se vzdy pamet dealokuje az dobehne
	// veskery kod, ktery objekt vyuziva
	
	// Vlakno se spustilo - tzn nastavime stav vlakna na bezici
	thread->Set_Running();

	// Provedeme spusteni programu (funkce TThread_Proc), coz vlakno zablokuje dokud nedobehne
	auto task_exit_code = static_cast<uint16_t>(thread->program(thread->regs));
	ProcessManager::Get().On_Thread_Finish(thread->tid, task_exit_code);
}

Thread::Thread(const kiv_os::TThread_Proc program, const kiv_hal::TRegisters context, const kiv_os::THandle tid,
               const kiv_os::THandle pid,
               const char* args, const bool is_main_thread): program(program), regs(context),
                                                             main_thread(is_main_thread),
                                                             args(args),
                                                             tid(tid), pid(pid) {
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(this->args.c_str());
}

std::thread::id Thread::Dispatch(std::shared_ptr<Thread> thread_ptr) {
	auto thread = std::thread(Thread_Func, std::move(thread_ptr));
	const auto thread_id = thread.get_id();
	thread.detach();
	return thread_id;
}

void Thread::Terminate_If_Running(const uint16_t exit_code) {
	if (task_state == TaskState::Running) {
		// Nastavime exit code a task state jako program terminated
		task_exit_code = exit_code;
		task_state = TaskState::Finished;
	}
}

TaskState Thread::GetState() const { return task_state; }

