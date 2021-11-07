#pragma once

#include "RunnableState.h"
#include "../api/api.h"
#include "Utils/Semaphore.h"


/// <summary>
/// Reprezentuje vlakno
/// </summary>
class Thread {

	RunnableState runnableState = RunnableState::Ready;

	/// <summary>
	/// Program, ktery bude vlakno vykonavat
	/// </summary>
	const kiv_os::TThread_Proc program;

	/// <summary>
	/// 
	/// </summary>
	kiv_hal::TRegisters regs;

	/// <summary>
	/// Zda-li se jedna o hlavni vlakno v procesu - pokud ano, proces se ukonci po dobehnuti
	/// </summary>
	bool is_main_thread;

	/// <summary>
	/// Semafor pro synchronizaci. Tento semafor se inicializuje na count = 0, aby se vlakno
	///	po init() bloklo a cekalo na spusteni
	/// </summary>
	Semaphore semaphore;

	kiv_os::THandle tid;

	/// <summary>
	/// Pid procesu, ke kteremu toto vlakno patri
	/// </summary>
	kiv_os::THandle pid;

	/// <summary>
	/// Funkce, ktera se vykonava ve vlakne
	/// </summary>
	void Thread_Func();


public:
	/// <summary>
	/// Spusti vlakno
	/// </summary>
	void Dispatch();

	Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context, kiv_os::THandle tid, kiv_os::THandle pid, bool is_main_thread = true)
	: program(program), regs(context), is_main_thread(is_main_thread), tid(tid), pid(pid) { }

	/// <summary>
	/// Provede inicializaci std::thread. Vlakno se blokne a vyckava, dokud se nezavola dispatch(),
	///	pote provede dany program 
	/// </summary>
	std::thread::id Init();

	[[nodiscard]] kiv_os::THandle Get_Tid() const { return tid; }
	[[nodiscard]] kiv_os::THandle Get_Pid() const { return pid; }
};
