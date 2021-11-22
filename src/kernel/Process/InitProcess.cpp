#include "InitProcess.h"

#include "kernel.h"
#include "IO/IOManager.h"

void CreateShell(kiv_hal::TRegisters& shell_regs, const kiv_os::THandle std_in_handle,
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

void WaitFor(kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_pid); // pointer na shell_pid
	regs.rcx.e = 1; // pouze jeden prvek
	Sys_Call(regs);
}

void ReadShellExitCode(const kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Read_Exit_Code);
	regs.rdx.x = shell_pid;
	Sys_Call(regs);
}

size_t InitProcess::InitFun(const kiv_hal::TRegisters& regs) {
	const auto std_in = regs.rax.x;
	const auto std_out = regs.rbx.x;

	auto shell_regs = kiv_hal::TRegisters();
	CreateShell(shell_regs, std_in, std_out);
	const auto shell_pid = shell_regs.rax.x;

	// Pockame az skonci shell
	WaitFor(shell_pid);

	// Pokud doslo k shutdownu ukoncime procesy
	if (ProcessManager::Get().shutdown_triggered) {
		PerformShutdown();
	}
	else {
		ReadShellExitCode(shell_pid); // Jinak staci precist exit code shellu
	}

	return 0; // Vlakno dobehne a protoze je main jedine co efektivne udela je, ze releasne semafor a main bude moci dobehnout
}

void InitProcess::PerformShutdown() {
	auto& process_manager = ProcessManager::Get();
	auto lock = std::scoped_lock(process_manager.mutex);
	LogDebug("Shutdown started. Locking down ProcessManager");
	const auto current_tid = process_manager.GetCurrentTid();
	const auto current_pid = process_manager.GetThread(current_tid)->GetPid();

	for (auto pid = ProcessManager::PID_RANGE_START; pid < ProcessManager::PID_RANGE_END; pid += 1) {
		if (current_pid != pid && process_manager.process_table[pid] != nullptr) {
			const auto process = process_manager.process_table[pid];
			process_manager.TerminateProcess(pid, true, -1);
			process_manager.RemoveProcessFromTable(process);
		}
	}
	
	// TODO zavrit otevrene soubory?
}

void InitProcess::NotifySubscribers(kiv_os::THandle task_id, bool terminated_forcefully) {
	// Vypneme stdio
	IOManager::Get().UnregisterProcessStdIO(std_in, std_out);
	semaphore->Release();
}

void InitProcess::Start() {
	ProcessManager::Get().RunInitProcess(InitFun);
	semaphore->Acquire(); // Zablokujeme main vlakno aby cekalo na shutdown
}

