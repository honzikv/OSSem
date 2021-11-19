#pragma once
#include "ProcessManager.h"
#include "../IO/IOManager.h"
#include "../Utils/Semaphore.h"



/// <summary>
/// Init proces pro vytvoreni shellu
/// </summary>
class InitProcess final : public Process {

	/// Chceme zdedit defaultni konstruktor
	using Process::Process;

	/// <summary>
	/// Semafor pro zablokovani
	/// </summary>
	inline static std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	/// <summary>
	/// Override abychom mohli ukoncit OS
	/// </summary>
	/// <param name="task_id"></param>
	/// <param name="called_from_task"></param>
	void NotifySubscribers(kiv_os::THandle task_id, bool called_from_task) override;


	/// <summary>
	/// Vytvori proces pro shell
	/// </summary>
	/// <param name="shell_regs">reference na registry (pro precteni pidu shellu)</param>
	/// <param name="std_in_handle">reference na stdin</param>
	/// <param name="std_out_handle">reference na stdout</param>
	static void CreateShell(kiv_hal::TRegisters& shell_regs, kiv_os::THandle std_in_handle,
		kiv_os::THandle std_out_handle);

	/// <summary>
	/// Proces pocka, dokud shell neskonci
	/// </summary>
	/// <param name="shell_pid"></param>
	static void WaitFor(kiv_os::THandle shell_pid);

	static void ReadShellExitCode(const kiv_os::THandle shell_pid);

	/// <summary>
	/// Funkce Init vlakna
	/// </summary>
	/// <param name="regs">registry musi obsahovat stdin, stdout a pid initu</param>
	/// <returns>vzdy success</returns>
	static size_t InitFun(const kiv_hal::TRegisters& regs);

	static void PerformShutdown(kiv_os::THandle this_thread_pid);
	

public:
	/// <summary>
	/// Spusti init proces, ktery bude blokovat dokud se nevypne shell
	/// </summary>
	static void Start();

};
