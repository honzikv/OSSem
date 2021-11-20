#pragma once

#include "kernel.h"

#include <csignal>
#include <Windows.h>

#include "IO/IOManager.h"
#include "Process/InitProcess.h"
#include "Process/ProcessManager.h"
#include "Utils/Debug.h"


void SysCall(kiv_hal::TRegisters& regs) {
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

void HandleSignal(int signum) {
	LogDebug("Hello signal");
}


void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	InitializeKernel();
	signal(SIGINT, HandleSignal);
	Set_Interrupt_Handler(kiv_os::System_Int_Number, SysCall);

	// Spustime init proces
	InitProcess::Start();

#if IS_DEBUG
	LogDebug("DEBUG: Init process killed. The Kernel will shutdown in 5s");
	std::this_thread::sleep_for(std::chrono::seconds(5));
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
