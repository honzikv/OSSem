#pragma once

#include "kernel.h"

#include <csignal>
#include <Windows.h>

#include "IO/IOManager.h"
#include "Process/InitProcess.h"
#include "Process/ProcessManager.h"
#include "Utils/Debug.h"


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


void InitializeKernel() {
	User_Programs = LoadLibraryW(L"user.dll");
}

void ShutdownKernel() {
	FreeLibrary(User_Programs);
}


void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	InitializeKernel();
	Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	// Spustime init proces
	InitProcess::Start();

#if IS_DEBUG
#endif

	ShutdownKernel();
}


void SetError(const bool failed, kiv_hal::TRegisters& regs) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.r = GetLastError();
	}
	else {
		regs.flags.carry = false;
	}
}
