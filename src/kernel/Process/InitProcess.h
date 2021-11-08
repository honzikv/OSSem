#pragma once
#include "ProcessManager.h"


size_t process_func(const kiv_hal::TRegisters& context);

/// <summary>
/// Pro prehlednost je cast kodu pro init proces rozdelena mimo ProcessManager.h a je pro ProcessManager friend class
/// </summary>
class InitProcess {

	
};
