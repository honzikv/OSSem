#pragma once

#include "kernel.h"

#include <csignal>
#include <Windows.h>

#include "IO/IOManager.h"
#include "Process/InitProcess.h"
#include "Process/ProcessManager.h"
#include "Utils/Debug.h"


void Syscall(kiv_hal::TRegisters& regs) {
	switch (static_cast<kiv_os::NOS_Service_Major>(regs.rax.h)) {
	case kiv_os::NOS_Service_Major::File_System:
		IOManager::Get().Handle_IO(regs);
		break;

	case kiv_os::NOS_Service_Major::Process:
		ProcessManager::Get().Syscall(regs);
		break;
	}

}

void Disable_Default_Behavior(int _) {
	// Override defaultniho chovani CTRL C / CTRL D
}

void Init_Signal_Behavior() {
	signal(SIGINT, Disable_Default_Behavior);
	raise(SIGINT); // timto signal zavolame a dalsi volani uz bude brat klavesnice misto handleru
}

void InitializeKernel() {
	User_Programs = LoadLibraryW(L"user.dll");
}

void ShutdownKernel() {
	FreeLibrary(User_Programs);
}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters& context) {
	InitializeKernel();
	Init_Signal_Behavior();
	IOManager::Get().Init_Filesystems();

	Set_Interrupt_Handler(kiv_os::System_Int_Number, Syscall);
	// Spustime init proces
	InitProcess::Start();

#if IS_DEBUG
	// LogDebug("DEBUG: Init process killed. The Kernel will shutdown in 5s");
	// std::this_thread::sleep_for(std::chrono::seconds(5));
#endif

	// Pokud jsme se dostali az sem OS se bude vypinat
	// Zavolame OnShutdown process manageru, coz nam sesynchronizuje main s ukoncenim ostatnich procesu
	ProcessManager::Get().On_Shutdown();
	ShutdownKernel();
}


void Set_Err(const bool failed, kiv_hal::TRegisters& regs) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.r = GetLastError();
	}
	else {
		regs.flags.carry = false;
	}
}
