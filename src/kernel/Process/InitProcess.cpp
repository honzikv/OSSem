#include "InitProcess.h"

#include "kernel.h"
#include "../IO/IOManager.h"

void Create_Shell(kiv_hal::TRegisters& shell_regs, const kiv_os::THandle std_in_handle,
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

void Wait_For(kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_pid); // pointer na shell_pid
	regs.rcx.e = 1; // pouze jeden prvek
	SysCall(regs);
}

void Read_Shell_Exit_Code(const kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Read_Exit_Code);
	regs.rdx.x = shell_pid;
	SysCall(regs);
}

size_t InitProcess::InitFun(const kiv_hal::TRegisters& regs) {
	const auto std_in = regs.rax.x;
	const auto std_out = regs.rbx.x;

	auto shell_regs = kiv_hal::TRegisters();
	Create_Shell(shell_regs, std_in, std_out);
	const auto shell_pid = shell_regs.rax.x;

	// Pockame az skonci shell
	Wait_For(shell_pid);
	Read_Shell_Exit_Code(shell_pid); // Jinak staci precist exit code shellu

	return 0;
}

void InitProcess::Start() {
	ProcessManager::Get().Run_Init_Process(InitFun);
	semaphore->Acquire(); // Zablokujeme main vlakno aby cekalo na shutdown
}

void InitProcess::Notify_Subscribers(const kiv_os::THandle this_task_handle) {
	Process::Notify_Subscribers(this_task_handle);
	semaphore->Release();
}

