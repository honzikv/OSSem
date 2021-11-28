#include "md.h"

size_t __stdcall md(const kiv_hal::TRegisters& regs) {
	uint16_t exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Success);

	const auto args = reinterpret_cast<char*>(regs.rdi.r);
	auto flags = static_cast<kiv_os::NOpen_File>(0);

	kiv_os::THandle handle;

	const std::string final_path(args);
	kiv_os_rtl::Open_File(handle, final_path, flags, kiv_os::NFile_Attributes::Directory);
	kiv_os_rtl::Close_File_Descriptor(handle);

	kiv_os_rtl::Exit(exit_code);
	return 0;
}