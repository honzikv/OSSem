#include "Thread.h"
#include "ProcessManager.h"
#include <csignal>

void Thread::ThreadFunc() {
	SetRunning();
	auto thread_tid = tid;
	auto thread_pid = pid;

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

/// <summary>
/// Windows API funkce pro spusteni vlakna
/// </summary>
/// <param name="params"></param>
/// <returns></returns>
DWORD WINAPI WinThreadFunc(const LPVOID params) {
	auto& thread = *static_cast<Thread*>(params);
	thread.ThreadFunc();
	return thread.GetExitCode();
}


std::pair<HANDLE, DWORD> Thread::Dispatch() {
	DWORD thread_id;
	auto thread_handle = CreateThread(nullptr, 0, WinThreadFunc, this, 0, &thread_id);
	return {thread_handle, thread_id};
}

void Thread::TerminateIfRunning(HANDLE handle, const uint16_t exit_code) {
	auto lock = std::scoped_lock(mutex);
	if (task_state != TaskState::Finished) {
		const auto result = TerminateThread(handle, exit_code);
		task_exit_code = exit_code;
	}
	task_state = TaskState::Finished;
}

TaskState Thread::GetState() const { return task_state; }
