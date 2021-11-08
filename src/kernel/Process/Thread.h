#pragma once

#include <vector>

#include "Task.h"
#include "TaskState.h"
#include "../api/api.h"
#include "Utils/Semaphore.h"


/// <summary>
/// Reprezentuje vlakno
/// </summary>
class Thread : public Task {

	/// <summary>
	/// Program, ktery bude vlakno vykonavat
	/// </summary>
	const kiv_os::TThread_Proc program;

	/// <summary>
	/// Kontext vlakna
	/// </summary>
	kiv_hal::TRegisters regs;

	/// <summary>
	/// Zda-li se jedna o hlavni vlakno v procesu - pokud ano, proces se ukonci po dobehnuti
	/// </summary>
	bool is_main_thread;

	/// <summary>
	/// Tid vlakna
	/// </summary>
	const kiv_os::THandle tid;

	/// <summary>
	/// Pid procesu, ke kteremu toto vlakno patri
	/// </summary>
	const kiv_os::THandle pid;

	/// <summary>
	/// Funkce, ktera se vykonava ve vlakne
	/// </summary>
	void ThreadFunc();


public:

	Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context, kiv_os::THandle tid, kiv_os::THandle pid, bool is_main_thread = true)
	: program(program), regs(context), is_main_thread(is_main_thread), tid(tid), pid(pid) { }

	/// <summary>
	/// Vytvori nativni vlakno s funkci Thread_Func() a vrati jeho id
	/// </summary>
	std::thread::id Dispatch();

	[[nodiscard]] inline kiv_os::THandle GetTid() const { return tid; }
	[[nodiscard]] inline kiv_os::THandle GetPid() const { return pid; }

	[[nodiscard]] TaskState GetState();
};
