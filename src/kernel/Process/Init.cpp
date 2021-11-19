#include "Init.h"
#include "kernel.h"

void InitProcess::CreateShell(kiv_hal::TRegisters& shell_regs, const kiv_os::THandle std_in_handle,
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
	SysCall(shell_regs);
}

void InitProcess::WaitFor(kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_pid); // pointer na shell_pid
	regs.rcx.e = 1; // pouze jeden prvek
	SysCall(regs);
}

void InitProcess::ReadShellExitCode(const kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Read_Exit_Code);
	regs.rdx.x = shell_pid;
	SysCall(regs);
}

void InitProcess::NotifySubscribers(kiv_os::THandle task_id, bool called_from_task) {
	semaphore->Release(); // Pouze odblokujeme semafor
}

size_t InitProcess::InitFun(const kiv_hal::TRegisters& regs) {
	const auto std_in = regs.rax.x;
	const auto std_out = regs.rbx.x;
	const auto this_thread_pid = regs.rcx.x; // Pid procesu se ulozi v registrech vzdy do rcx

	auto shell_regs = kiv_hal::TRegisters();
	CreateShell(shell_regs, std_in, std_out);
	const auto shell_pid = shell_regs.rax.x;

	// Nastavime callback na shell
	// Nicmene se muzeme vzbudit i pokud nekdo zavola shutdown
	WaitFor(shell_pid);

	if (ProcessManager::Get().shutdown_triggered) {
		// Pokud se provedl shutdown musime povypinat vsechny procesy
		ProcessManager::Get().TerminateOtherProcesses(this_thread_pid);
	}
	else {
		ReadShellExitCode(shell_pid); // jinak pouze odstranime shell z tabulky
	}

	return 0; // Po ukonceni se zavola notify subscribers, ktere vzbudi "boot" vlakno
}


void InitProcess::Start() {
	ProcessManager::Get().RunInitProcess(InitFun);
	semaphore->Acquire(); // Blokneme proces a init ho pak odblokuje po dobehnuti
}
