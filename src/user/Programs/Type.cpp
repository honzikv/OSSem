#include "type.h"
#include "rtl.h"

size_t __stdcall type(const kiv_hal::TRegisters& regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	size_t read = 1;

	constexpr size_t read_buffer_size = 1024;
	std::vector<char> read_buffer(read_buffer_size);

	kiv_os::THandle handler_in;
	std::string output;

	if (args.empty()) {
		const std::string message("Missing argument.\n");

		size_t written;
		kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument));
		return 1;
	}

	kiv_os::NFile_Attributes file_attributes;
	kiv_os_rtl::Get_File_Attributes(args, file_attributes);
	if (file_attributes == kiv_os::NFile_Attributes::Directory) {
		// je to dir -> nebudu vypisovat
		const std::string message("Access denied.\n");

		size_t written;
		kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument));
		return 1;
	}

	if (!kiv_os_rtl::Open_File(handler_in, args, kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::System_File)) {
		// soubor se neotevrel
		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found));
		return 1;
	}

    // Nastavi pozici v souboru na zacatek
	size_t position_start = 0;
    kiv_os_rtl::Seek(handler_in, kiv_os::NFile_Seek::Set_Position, kiv_os::NFile_Seek::Beginning, position_start);

	while (read) {
		if (!kiv_os_rtl::Read_File(handler_in, read_buffer.data(), read_buffer_size, read)) {
			// je to dir -> nebudu vypisovat
			const std::string message("IO Error.\n");

			size_t written;
			kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
			kiv_os_rtl::Exit(static_cast<uint16_t>(read));
			return 1;
		}

		output.append(read_buffer.data(), read);
		if (read < read_buffer_size) {
			break;
		}
	}
	if (output.empty()) {
		output.append("\n");
	}

	size_t written;
	kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);

	kiv_os_rtl::Close_File_Descriptor(handler_in);

	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));

	return 0;
}