#include <string>

#include "Rd.h"
#include "../rtl.h"

size_t __stdcall rd(const kiv_hal::TRegisters& regs) {
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto final_path = std::string(reinterpret_cast<char*>(regs.rdi.r));

	if (final_path.empty()) {
		const std::string message("Missing argument.\n");

		size_t written;
		kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument));
		return 1;
	}
	if (!kiv_os_rtl::Delete_File(final_path)) {
		const std::string message("Can not delete.\n");

		size_t written;
		kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::IO_Error));
		return 1;
	}

	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}
