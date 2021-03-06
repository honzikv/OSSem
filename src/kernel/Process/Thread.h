#pragma once

#include <string>

#include "Task.h"
#include "TaskState.h"
#include "../../api/api.h"
#include "../Utils/Logging.h"
#include "../Utils/Semaphore.h"


/// <summary>
/// Reprezentuje vlakno
/// </summary>
class Thread final : public Task {

	/// <summary>
	/// Program, ktery bude vlakno vykonavat
	/// </summary>
	const kiv_os::TThread_Proc program;

	/// <summary>
	/// Mutex, ktery zabezpecuje ze nelze zaroven vlakno vypnout odstrelenim pres TerminateThread z jineho vlakna
	///	a prirozene dobehnutim
	/// </summary>
	std::recursive_mutex thread_finish_mutex = {};

	/// <summary>
	/// Kontext vlakna
	/// </summary>
	kiv_hal::TRegisters regs;

	/// <summary>
	/// Zda-li se jedna o hlavni vlakno v procesu - pokud ano, proces se ukonci po dobehnuti
	/// </summary>
	bool main_thread;

	/// <summary>
	/// Argumenty programu
	/// </summary>
	std::string args;

	/// <summary>
	/// Tid vlakna
	/// </summary>
	const kiv_os::THandle tid;

	/// <summary>
	/// Pid procesu, ke kteremu toto vlakno patri
	/// </summary>
	const kiv_os::THandle pid;


public:
	/// <summary>
	/// Konstruktor pro CreateProcess
	/// </summary>
	/// <param name="program">funkce, ktera se spusti pri behu vlakna</param>
	/// <param name="context">Kontext registru pri behu vlakna</param>
	/// <param name="tid">tid vlakna</param>
	/// <param name="pid">pid procesu, ktery vlakno vlastni</param>
	/// <param name="args">Argumenty programu</param>
	/// <param name="is_main_thread">Zda-li je vlakno hlavni</param>
	Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context, kiv_os::THandle tid, kiv_os::THandle pid,
	       const char* args,
	       bool is_main_thread = true);
	/// <summary>
	/// Konstruktor pro CreateThread
	/// </summary>
	/// <param name="program">funkce, ktera se spusti pri behu vlakna</param>
	/// <param name="context">Kontext registru pri behu vlakna</param>
	/// <param name="tid">tid vlakna</param>
	/// <param name="pid">pid procesu, ktery vlakno vlastni</param>
	Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context, kiv_os::THandle tid, kiv_os::THandle pid);

	/// <summary>
	/// Vytvori nativni vlakno s funkci Thread_Func() a vrati jeho handle a thread id
	/// </summary>
	[[nodiscard]] static std::thread::id Dispatch(std::shared_ptr<Thread> thread_ptr) ;

	/// <summary>
	/// Nasilne ukonci vlakno. Tato funkce nic nedela, pokud se vlakno ukoncilo samo
	/// </summary>
	/// <param name="native_thread_handle"></param>
	/// <param name="exit_code">Exit code, ktery se nastavi po ukonceni</param>
	void Terminate_If_Running(uint16_t exit_code);

	/// <summary>
	/// Funkce, ktera se vykonava ve vlakne
	/// </summary>
	friend static void Thread_Func(std::shared_ptr<Thread> thread);

	[[nodiscard]] inline kiv_os::THandle Get_Tid() const { return tid; }
	[[nodiscard]] inline kiv_os::THandle Get_Pid() const { return pid; }
	[[nodiscard]] inline bool Is_Main_Thread() const { return main_thread; }

	[[nodiscard]] TaskState GetState() const;

};
