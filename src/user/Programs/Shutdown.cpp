#include "Shutdown.h"
size_t shutdown(const kiv_hal::TRegisters& regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rbx.x);
	kiv_os_rtl::Close_File_Descriptor(std_in); // toto musime udelat, jinak cokoliv co ceka na precteni nam zablokuje shutdown
	kiv_os_rtl::Shutdown();
	return 0;
}
