#include "Shutdown.h"
#include "../rtl.h"
#include "../Utils/Logging.h"

size_t  __stdcall  shutdown(const kiv_hal::TRegisters& regs) {
	kiv_os_rtl::Shutdown();
	Log_Debug("Shutdown finished");
	return 0;
}
