#pragma once

#include "kernel.h"

#include "io.h"
#include <Windows.h>

#include "IO/ConsoleIn.h"
#include "IO/ConsoleOut.h"
#include "Process/RunnableManager.h"


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
			RunnableManager::get().Process_Syscall(regs);
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

	// Vytvorime shell
	auto regs = kiv_hal::TRegisters();
	const auto shell_command = "shell";
	const auto shell_args = "";

	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Clone);
	regs.rcx.l = static_cast<decltype(regs.rcx.l)>(kiv_os::NClone::Create_Process);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(shell_command); // rdx je pretypovany pointer na jmeno souboru
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(shell_args); // rdi je pointer na argumenty
	regs.rbx.e = std_in_handle << 16 | std_out_handle; // rbx obsahuje stdin a stdout

	Sys_Call(regs);

	while (1) {
		
	}

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
