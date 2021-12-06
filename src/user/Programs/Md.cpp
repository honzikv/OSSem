#include "Md.h"

#include <string>

#include "../rtl.h"

size_t __stdcall md(const kiv_hal::TRegisters& regs) {
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto args = std::string(reinterpret_cast<char*>(regs.rdi.r));

	kiv_os::THandle handle;

	if (args.empty()) {
		const std::string message("Missing argument.\n");

		size_t written;
		kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument));
		return 1;
	}
	
	if (!kiv_os_rtl::Open_File(handle, args, 
							   static_cast<kiv_os::NOpen_File>(0), kiv_os::NFile_Attributes::Directory)) {
		// slozka se nevytvorila
		const std::string message("Dir can not be created.\n");

		size_t written;
		kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found));
		return 1;
	}
	kiv_os_rtl::Close_File_Descriptor(handle);

	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}
