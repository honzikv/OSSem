#include "Shutdown.h"
size_t shutdown(const kiv_hal::TRegisters& regs) {
	kiv_os_rtl::Shutdown();
	return 0;
}
