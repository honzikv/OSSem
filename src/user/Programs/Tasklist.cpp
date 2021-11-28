#include "Tasklist.h"

size_t __stdcall tasklist(const kiv_hal::TRegisters &regs) {
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	constexpr size_t buffer_size = 256;
	std::vector<char> buffer(buffer_size);
	std::string output;
	size_t read;

	kiv_os::THandle handler;
	kiv_os_rtl::Open_File(handler, "procfs", static_cast<kiv_os::NOpen_File>(0), static_cast<kiv_os::NFile_Attributes>(0));
	kiv_os_rtl::Read_File(handler, buffer.data(), buffer_size, read);

	size_t written;
	kiv_os_rtl::Write_File(std_out, buffer.data(), read, written);

	kiv_os_rtl::Close_File_Descriptor(handler);

	kiv_os_rtl::Write_File(std_out, "\n", 1, written);

	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}