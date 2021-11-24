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
	static size_t Init_Fun(const kiv_hal::TRegisters& regs);

public:
	/// <summary>
	/// Spusti Init proces
	/// </summary>
	static void Start();

	void Notify_Subscribers(kiv_os::THandle this_task_handle) override;
	
};
