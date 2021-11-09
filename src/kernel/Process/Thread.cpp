#include "Thread.h"
#include "ProcessManager.h"


void Thread::ThreadFunc() {
	SetRunning();
	task_exit_code = static_cast<kiv_os::NOS_Error>(program(regs)); // ziskame exit code z programu

	// Program dobehl, takze muzeme notifikovat vsechny subscribery
	Finish(tid);

	// Pokud je toto vlakno main, ukoncime i proces
	if (is_main_thread) {
		ProcessManager::Get().FinishProcess(pid);
	}

}

Thread::Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context, kiv_os::THandle tid, kiv_os::THandle pid,
               const char* args, bool is_main_thread): program(program), regs(context), is_main_thread(is_main_thread),
                                                       args(args),
                                                       tid(tid), pid(pid) {
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(this->args.c_str());
}


std::thread::id Thread::Dispatch() {
	auto thread = std::thread(&Thread::ThreadFunc, this);
	const auto thread_id = thread.get_id();
	thread.detach(); // vlakno detachneme
	return thread_id; // vratime thread id
}

TaskState Thread::GetState() { return task_state; }
