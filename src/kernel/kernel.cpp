#pragma once

#include "kernel.h"

#include "io.h"
#include <Windows.h>

#include "IO/ConsoleIn.h"
#include "IO/ConsoleOut.h"
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
			Handle_IO(regs);
			break;

		case kiv_os::NOS_Service_Major::Process:
			ProcessManager::Get().Process_Syscall(regs);
			break;
	}

}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	Initialize_Kernel();

	// Vytvorime stdin a stdout
	auto stdIn = ConsoleIn();
	auto stdOut = ConsoleOut();

	// Vytvorime handle pro stdIn a stdOut
	const auto std_in_handle = Convert_Native_Handle(&stdIn);
	const auto std_out_handle = Convert_Native_Handle(&stdOut);

	Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	// Vytvorime "fake" Init proces
	ProcessManager::Get().Create_Init_Process();

	// Vytvorime shell
	const auto shell_command = "shell";
	const auto shell_args = "";

	auto shell_regs = kiv_hal::TRegisters();
	shell_regs.rax.h = static_cast<decltype(shell_regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	shell_regs.rax.l = static_cast<decltype(shell_regs.rax.l)>(kiv_os::NOS_Process::Clone);
	shell_regs.rcx.l = static_cast<decltype(shell_regs.rcx.l)>(kiv_os::NClone::Create_Process);
	shell_regs.rdx.r = reinterpret_cast<decltype(shell_regs.rdx.r)>(shell_command);
	// rdx je pretypovany pointer na jmeno souboru
	shell_regs.rdi.r = reinterpret_cast<decltype(shell_regs.rdi.r)>(shell_args); // rdi je pointer na argumenty
	shell_regs.rbx.e = std_in_handle << 16 | std_out_handle; // rbx obsahuje stdin a stdout
	Sys_Call(shell_regs);


	// Nyni musime blokovat v "tomto" vlakne, dokud shell neskonci
	const auto shell_pid = shell_regs.rax.x;
	auto init_regs = kiv_hal::TRegisters();
	init_regs.rax.h = static_cast<decltype(init_regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	init_regs.rax.l = static_cast<decltype(init_regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	init_regs.rdx.r = reinterpret_cast<decltype(init_regs.rdx.r)>(&shell_pid); // pointer na shell_pid
	init_regs.rcx.e = 1; // pouze jeden prvek
	Sys_Call(init_regs);

	// TODO remove
#if IS_DEBUG
	LogDebug("Init process has been killed. Kernel would die here if it wasn't in debug");
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
