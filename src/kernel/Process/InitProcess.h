#pragma once
#include "Process/ProcessManager.h"

class InitProcess final : public Process {
	using Process::Process;

	static inline std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	/// <summary>
	/// Funkce init procesu
	/// </summary>
	/// <param name="regs">registry s kontextem</param>
	/// <returns></returns>
	static size_t InitFun(const kiv_hal::TRegisters& regs);

	static void PerformShutdown();

	void NotifySubscribers(kiv_os::THandle task_id, bool terminated_forcefully) override;

public:
	/// <summary>
	/// Spusti Init proces
	/// </summary>
	static void Start();
	
};

class InitThread final : public Thread {
public:
	using Thread::Thread;
	void NotifySubscribers(kiv_os::THandle task_id, bool terminated_forcefully) override {
		// nic notifikovat nechceme
	}
};