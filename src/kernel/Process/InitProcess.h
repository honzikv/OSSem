#pragma once
#include "Process/ProcessManager.h"

class InitProcess {

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
	
};
