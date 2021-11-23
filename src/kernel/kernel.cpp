﻿#pragma once

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
		IOManager::Get().Handle_IO(regs);
		break;

	case kiv_os::NOS_Service_Major::Process:
		ProcessManager::Get().Syscall(regs);
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
	Log_Debug("Hello signal");
}


void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	InitializeKernel();
	signal(SIGINT, HandleSignal);
	Set_Interrupt_Handler(kiv_os::System_Int_Number, SysCall);

	// Spustime init proces
	InitProcess::Start();

#if IS_DEBUG
	// LogDebug("DEBUG: Init process killed. The Kernel will shutdown in 5s");
	// std::this_thread::sleep_for(std::chrono::seconds(5));
#endif

	// Pokud jsme se dostali az sem OS se bude vypinat
	// Zavolame OnShutdown process manageru, coz nam sesynchronizuje main s ukoncenim init procesu
	ProcessManager::Get().On_Shutdown();

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
