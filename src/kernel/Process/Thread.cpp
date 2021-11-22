#include "Thread.h"
#include "ProcessManager.h"
#include <csignal>

void Thread::ThreadFunc() {
	SetRunning();

	const auto task_exit_code = program(regs); // ziskame exit code z programu
	SetExitCode(task_exit_code); // nastavime ho

	// Program dobehl, rekneme process managerovi at ho ukonci
	ProcessManager::Get().OnThreadFinished(tid);

	if (main_thread) {
		ProcessManager::Get().OnProcessFinished(pid, task_exit_code);
	}
}

Thread::Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context, kiv_os::THandle tid, kiv_os::THandle pid,
               const char* args, bool is_main_thread): program(program), regs(context), main_thread(is_main_thread),
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

TaskState Thread::GetState() const { return task_state; }
