#include "Thread.h"
#include "ProcessManager.h"
#include <csignal>

void Thread::ThreadFunc() {
	// Vlakno se spustilo - tzn nastavime stav vlakna na bezici
	SetRunning();

	// Provedeme spusteni programu (funkce TThread_Proc), coz vlakno zablokuje dokud nedobehne
	// a ziskame exit code z programu
	const auto task_exit_code = program(regs);

	// Nastavime exit code - toto se nemusi synchronizovat, protoze bud se vlakno v tomto momentu ukonci nasilim
	// a exit code se nastavi na specialni a nebo se zavola callback pro notifikaci
	this->task_exit_code = task_exit_code;

	// Zavolame funkci pro ukonceni vlakna v process manageru
	// Zde musime objekt zamknout aby neslo zaroven vlakno odstrelit
	auto lock = std::scoped_lock(thread_finish_mutex);
	if (task_state == TaskState::ProgramTerminated) {
		return;
	}
	task_state = TaskState::ProgramFinished;
	ProcessManager::Get().OnThreadFinish(tid);
}

Thread::Thread(const kiv_os::TThread_Proc program, kiv_hal::TRegisters context, const kiv_os::THandle tid,
               const kiv_os::THandle pid,
               const char* args, const bool is_main_thread): program(program), regs(context), main_thread(is_main_thread),
                                                             args(args),
                                                             tid(tid), pid(pid) {
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(this->args.c_str());
}

/// <summary>
/// Windows API funkce pro spusteni vlakna
/// </summary>
/// <param name="params"></param>
/// <returns></returns>
// DWORD WINAPI WinThreadFunc(const LPVOID params) {
// 	auto& thread = *static_cast<Thread*>(params);
// 	thread.ThreadFunc();
// 	return thread.GetTaskExitCode();
// }


std::thread::id Thread::Dispatch() {
	// DWORD thread_id;
	// auto thread_handle = CreateThread(nullptr, 0, WinThreadFunc, this, 0, &thread_id);
	// return {thread_handle, thread_id};

	auto thread = std::thread(&Thread::ThreadFunc, this);
	const auto thread_id = thread.get_id();

	thread.detach();
	return thread_id;
}

void Thread::TerminateIfRunning(const uint16_t exit_code) {
	auto lock = std::scoped_lock(thread_finish_mutex);
	// Pokud vlakno porad bezi, pak jsme se dostali jeste pred situaci, nez se vlakno ukoncilo prirozene
	// Tzn. vlakno lze bezpecne zabit pomoci Windows api
	if (task_state == TaskState::Running) {
		// TerminateThread(native_thread_handle, exit_code);

		// Nastavime exit code a task state jako program terminated
		task_exit_code = exit_code;
		task_state = TaskState::ProgramTerminated;
	}
}

TaskState Thread::GetState() const { return task_state; }
