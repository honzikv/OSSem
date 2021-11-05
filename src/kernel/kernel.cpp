#pragma once

#include "kernel.h"

#include <iostream>
#include <ostream>

#include "io.h"
#include <Windows.h>

#include "handles.h"
#include "IO/ConsoleIn.h"
#include "IO/ConsoleOut.h"
#include "Process/ProcessManager.h"

static auto processManager = ProcessManager();

HMODULE User_Programs;

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
			processManager.serveProcess(regs);
			break;
	}

}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	Initialize_Kernel();

	// Vytvorime stdin a stdout
	auto stdIn = ConsoleIn();
	auto stdOut = ConsoleOut();

	// Vytvorime handle pro stdIn a stdOut
	const auto stdInHandle = Convert_Native_Handle(&stdIn);
	const auto stdOutHandle = Convert_Native_Handle(&stdOut);

	// Vytvorime shell, ktery bude blokovat, dokud se nevypne pres ctrl+c, exit, apod.


	const auto stdInFromHandle = static_cast<AbstractFile*>(Resolve_kiv_os_Handle(stdOutHandle));
	const auto message = std::string("Hello Kernel\n");
	uint32_t bytesWritten;
	stdInFromHandle->write(message.c_str(), message.size(), bytesWritten);

	std::cout << stdInHandle << ", " << stdOutHandle << std::endl;
	int x;
	std::cin >> x;

	Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);


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
