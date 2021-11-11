#pragma once

#include "kernel.h"

#include "io.h"
#include <Windows.h>

#include "IO/ConsoleIn.h"
#include "IO/ConsoleOut.h"
#include "IO/IOManager.h"
#include "Process/ProcessManager.h"
#include "Utils/Config.h"


void Initialize_Kernel() {
	User_Programs = LoadLibraryW(L"user.dll");
}

void Shutdown_Kernel() {
	FreeLibrary(User_Programs);
}

/// <summary>
/// Funkce pro systemove volani
/// </summary>
/// <param name="regs">registry s kontextem</param>
void __stdcall Sys_Call(kiv_hal::TRegisters& regs) {
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
void CreateShell(kiv_hal::TRegisters& shell_regs, const kiv_os::THandle std_in_handle, const kiv_os::THandle std_out_handle) {
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
void WaitForShell(kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_pid); // pointer na shell_pid
	regs.rcx.e = 1; // pouze jeden prvek
	Sys_Call(regs);
}

void RemoveShellProcess(const kiv_os::THandle shell_pid) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Exit);
	regs.rdx.x = shell_pid;
	Sys_Call(regs);
}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	Initialize_Kernel();
	
	// Nechame si vytvorit stdin a stdout
	const auto [std_in_handle, std_out_handle] = IOManager::Get().CreateStdIO();

	Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	// Vytvorime "fake" Init proces
	ProcessManager::Get().CreateInitProcess();

	auto shell_regs = kiv_hal::TRegisters();
	CreateShell(shell_regs, std_in_handle, std_out_handle);

	// Nyni musime blokovat v "tomto" vlakne, dokud shell neskonci
	const auto shell_pid = shell_regs.rax.x;
	WaitForShell(shell_pid);
	RemoveShellProcess(shell_pid);

	// TODO remove this debug
#if IS_DEBUG
	LogDebug("DEBUG: Init killed. Stopped before Kernel Shutdown");
	while (true) { }
#endif

	Shutdown_Kernel();
}


void Set_Error(const bool failed, kiv_hal::TRegisters& regs) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.r = GetLastError();
	}
	else {
		regs.flags.carry = false;
	}
}
