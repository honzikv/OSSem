#pragma once
#include <memory>

#include "Process.h"
#include "../api/api.h"
#include <array>
#include <iostream>
#include <string>
#include <mutex>
#include <ostream>

#include "Thread.h"
/**
 * Trida, ktera ma na starost spravu jednotlivych procesu v OS
 */
class ProcessManager {
public:
	/**
	 * Konstanty pro rozsahy id pro process id a thread id
	 */
	static constexpr uint16_t PID_RANGE_START = 0;
	static constexpr uint16_t PID_RANGE_END = 4095;
	static constexpr uint16_t TID_RANGE_START = 4096;
	static constexpr uint16_t TID_RANGE_END = 8144;
	static constexpr uint16_t NO_FREE_ID = -1;
	inline static const std::string DEFAULT_PROCESS_WORKDIR = "C:\\";  // NOLINT(clang-diagnostic-exit-time-destructors)
	
private:
	/**
	 * "Tabulka procesu", index je process id.
	 */
	std::array<std::shared_ptr<Process>, PID_RANGE_END - PID_RANGE_START> processTable;

	/**
	 * Tabulka vlaken, index je thread id
	 */
	std::array<std::shared_ptr<Thread>, TID_RANGE_END - TID_RANGE_START> threadTable;

	/**
	 * Ziska volny pid, pokud neni vrati NO_FREE_ID
	 */
	kiv_os::THandle getFreePid();

	/**
	 * Ziska volny tid, pokud neni vrati NO_FREE_ID
	 */
	kiv_os::THandle getFreeTid();

	/**
	 * Ziska shared pointer na proces podle pidu
	 */
	std::shared_ptr<Process> getProcess(const kiv_os::THandle pid);

	/**
	 * Ziska shared pointer na vlakno podle tidu
	 */
	std::shared_ptr<Thread> getThread(const kiv_os::THandle tid) {
		return threadTable[tid + TID_RANGE_START];
	}


	std::mutex processTableMutex;
	std::mutex threadTableMutex;

public:
	/**
	 * Obsluha pri zadosti o praci s procesem
	 */
	kiv_os::NOS_Error serveProcess(kiv_hal::TRegisters& regs) {
		auto operation = regs.rax.l;
		switch (static_cast<kiv_os::NOS_Process>(operation)) {
		case kiv_os::NOS_Process::Clone:
			return performClone(regs);

		case kiv_os::NOS_Process::Wait_For:
			return performWaitFor(regs);

		case kiv_os::NOS_Process::Read_Exit_Code:
			return performReadExitCode(regs);

		case kiv_os::NOS_Process::Exit:
			return performProcessExit(regs);

		case kiv_os::NOS_Process::Shutdown:
			return performShutdown(regs);

		case kiv_os::NOS_Process::Register_Signal_Handler:
			return performRegisterSignalHandler(regs);
		}
	}

	kiv_os::NOS_Error performClone(kiv_hal::TRegisters& regs) {
		const auto operationType = static_cast<kiv_os::NClone>(regs.rcx.l);

		if (operationType == kiv_os::NClone::Create_Process) {
			return createNewProcess(regs);
		}
		if (operationType == kiv_os::NClone::Create_Thread) {
			return createNewThread(regs);
		}

		// Jinak vratime invalid argument
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	kiv_os::NOS_Error createNewProcess(kiv_hal::TRegisters& regs) {
		// Nejprve provedeme check zda-li muzeme vytvorit novy proces - musi existovat volny pid
		const auto newPid = getFreePid();
		if (newPid == NO_FREE_ID) {
			return kiv_os::NOS_Error::Out_Of_Memory; // OOM
		}

		const auto programName = std::string(reinterpret_cast<char*>(regs.rdx.r)); // NOLINT(performance-no-int-to-ptr)
		const auto programArgs = std::string(reinterpret_cast<char*>(regs.rdi.r)); // NOLINT(performance-no-int-to-ptr)

		// Ziskame stdout a stdin z registru bx
		const auto stdOut = static_cast<kiv_os::THandle>(regs.rbx.x);
		const auto stdIn = static_cast<kiv_os::THandle>(regs.rbx.x << 16);


	}
	// TODO impl this
	kiv_os::NOS_Error createNewThread(kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
	kiv_os::NOS_Error performWaitFor(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
	kiv_os::NOS_Error performReadExitCode(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
	kiv_os::NOS_Error performProcessExit(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
	kiv_os::NOS_Error performShutdown(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
	kiv_os::NOS_Error performRegisterSignalHandler(const kiv_hal::TRegisters& regs) {
		return kiv_os::NOS_Error::Success;
	}
};

inline auto processManager = ProcessManager();