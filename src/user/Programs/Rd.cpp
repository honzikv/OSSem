#include "rd.h"

size_t __stdcall rd(const kiv_hal::TRegisters& regs) {
	const auto exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Success);

	char* file_name = reinterpret_cast<char*>(regs.rdi.r);

	std::string final_path(file_name);

	kiv_os_rtl::Delete_File(file_name);

	kiv_os_rtl::Exit(exit_code);
	return 0;
}