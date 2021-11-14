#pragma once

#include "kernel.h"

#include "io.h"
#include <Windows.h>

#include "IO/ConsoleIn.h"
#include "IO/ConsoleOut.h"
#include "IO/IOManager.h"
#include "Process/Init.h"
#include "Process/ProcessManager.h"
#include "Utils/Config.h"


void Initialize_Kernel() {
	User_Programs = LoadLibraryW(L"user.dll");
}

void Shutdown_Kernel() {
	FreeLibrary(User_Programs);
}



void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	Initialize_Kernel();
	Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	// Spustime init proces
	InitProcess::Dispatch();
	
	// TODO remove this debug
#if IS_DEBUG
	LogDebug("DEBUG: Init process killed. Stopped before Kernel Shutdown");
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
