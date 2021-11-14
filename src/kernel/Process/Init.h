#pragma once
#include "ProcessManager.h"
#include "../IO/IOManager.h"
#include "../Utils/Semaphore.h"


/// <summary>
/// Funkce pro systemove volani
/// </summary>
/// <param name="regs">registry s kontextem</param>
inline void __stdcall Sys_Call(kiv_hal::TRegisters& regs) {
	switch (static_cast<kiv_os::NOS_Service_Major>(regs.rax.h)) {
		case kiv_os::NOS_Service_Major::File_System:
			IOManager::Get().HandleIO(regs);
			break;

		case kiv_os::NOS_Service_Major::Process:
			ProcessManager::Get().ProcessSyscall(regs);
			break;
	}

}

/// <summary>
/// Vytvori proces pro shell
/// </summary>
/// <param name="shell_regs">reference na registry (pro precteni pidu shellu)</param>
/// <param name="std_in_handle">reference na stdin</param>
/// <param name="std_out_handle">reference na stdout</param>
inline void CreateShell(kiv_hal::TRegisters& shell_regs, const kiv_os::THandle std_in_handle,
                        const kiv_os::THandle std_out_handle) {
	const auto shell_command = "shell";
	const auto shell_args = "";

	shell_regs.rax.h = static_cast<decltype(shell_regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	shell_regs.rax.l = static_cast<decltype(shell_regs.rax.l)>(kiv_os::NOS_Process::Clone);
	shell_regs.rcx.l = static_cast<decltype(shell_regs.rcx.l)>(kiv_os::NClone::Create_Process);
	// rdx je pretypovany pointer na jmeno souboru
	shell_regs.rdx.r = reinterpret_cast<decltype(shell_regs.rdx.r)>(shell_command);
	shell_regs.rdi.r = reinterpret_cast<decltype(shell_regs.rdi.r)>(shell_args); // rdi je pointer na argumenty
	shell_regs.rbx.e = std_in_handle << 16 | std_out_handle; // rbx obsahuje stdin a stdout
	Sys_Call(shell_regs);
}

/// <summary>
/// Proces pocka, dokud shell neskonci
/// </summary>
/// <param name="shell_pid"></param>
inline void WaitFor(kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_pid); // pointer na shell_pid
	regs.rcx.e = 1; // pouze jeden prvek
	Sys_Call(regs);
}

inline void RemoveShellProcess(const kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Exit);
	regs.rdx.x = shell_pid;
	Sys_Call(regs);
}


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

	static size_t InitFun(const kiv_hal::TRegisters& _) {

		// Nechame si vytvorit stdin a stdout
		const auto [std_in_handle, std_out_handle] = IOManager::Get().CreateStdIO();
		auto shell_regs = kiv_hal::TRegisters();
		CreateShell(shell_regs, std_in_handle, std_out_handle);
		const auto shell_pid = shell_regs.rax.x;
		// Nejprve cekame az skonci shell
		WaitFor(shell_pid);
		if (ProcessManager::Get().shutdown_triggered) {
			ProcessManager::Get().shutdown_semaphore->Acquire();
		}

		return 0; // Potom se vratime, odstrani se toto vlakno a zavola se notify subscribers, ktere odblokuje hlavni vlakno
	}

public:
	/// <summary>
	/// Spusti init proces, ktery bude blokovat dokud se nevypne shell
	/// </summary>
	static void Dispatch();


};
