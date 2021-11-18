#include "Init.h"

void Sys_Call(kiv_hal::TRegisters& regs) {
	switch (static_cast<kiv_os::NOS_Service_Major>(regs.rax.h)) {
		case kiv_os::NOS_Service_Major::File_System:
			IOManager::Get().HandleIO(regs);
			break;

		case kiv_os::NOS_Service_Major::Process:
			ProcessManager::Get().ProcessSyscall(regs);
			break;
	}

}

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

void InitProcess::NotifySubscribers(kiv_os::THandle task_id, bool called_from_task) {
	semaphore->Release(); // Pouze odblokujeme semafor
}

size_t InitProcess::InitFun(const kiv_hal::TRegisters& _) {
	// Nechame si vytvorit stdin a stdout
	const auto [std_in_handle, std_out_handle] = IOManager::Get().CreateStdIO();
	auto shell_regs = kiv_hal::TRegisters();
	CreateShell(shell_regs, std_in_handle, std_out_handle);
	const auto shell_pid = shell_regs.rax.x;
	// Nejprve cekame az skonci shell
	WaitFor(shell_pid);

	// Pokud se provedl shutdown musime pockat nez se povypinaji vsechna vlakna
	if (ProcessManager::Get().shutdown_triggered) {
		ProcessManager::Get().shutdown_semaphore->Acquire();
	}
	else {
		ReadShellExitCode(shell_pid);
	}

	return 0; // Potom se vratime, odstrani se toto vlakno a zavola se notify subscribers, ktere odblokuje hlavni vlakno
}

void InitProcess::Start() {
	ProcessManager::Get().RunInitProcess(InitFun);
	semaphore->Acquire();
}
