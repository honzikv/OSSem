#include "find.h"

size_t __stdcall find(const kiv_hal::TRegisters &regs) {
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const char *arguments = reinterpret_cast<const char *>(regs.rdi.r);

	std::string output;
	size_t written;

	if (strlen(arguments) == 0) {
		output = "Wrong arguments.\n";
		uint16_t exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		kiv_os_rtl::Exit(exit_code);
		return 0;
	}

	std::stringstream stream(arguments);

	std::string part1;
	std::string part2;
	std::string rest;
	std::string other = "";

	stream >> part1;
	stream >> part2;

	while (stream >> rest) {
		other.append(rest);
	}

	size_t actual_position = 0;

	if (part1 == "/v" && part2 == "/c\"\"") {
		kiv_os::THandle in_handle = std_in;
		bool is_file = false;
		if (!rest.empty()) {
			kiv_os_rtl::Open_File(in_handle, rest, kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::System_File);
			is_file = true;
			kiv_os_rtl::Seek(in_handle, kiv_os::NFile_Seek::Set_Position, kiv_os::NFile_Seek::Beginning, actual_position);
		}

		if (in_handle == static_cast<kiv_os::THandle>(-1)) {
			uint16_t exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found);
			kiv_os_rtl::Exit(exit_code);
			return 0;
		}
		size_t read = 1;

		constexpr int buffer_size = 256;
		std::vector<char> buffer(buffer_size);
		std::string complete;

		while (read) {
			kiv_os_rtl::Read_File(in_handle, buffer.data(), buffer_size, read);
			complete.append(buffer.data(), 0, read);

			if (buffer[0] == 4) {
				break;
			}

			if (is_file) {
				actual_position += read;
				kiv_os_rtl::Seek(in_handle, kiv_os::NFile_Seek::Set_Position, kiv_os::NFile_Seek::Beginning, actual_position);
			}
		}

		if (is_file) {
			kiv_os_rtl::Close_File_Descriptor(in_handle);
		}
		
		std::stringstream data(complete);
		std::string line;
		int lines = 0;
		while (std::getline(data, line)) {
			lines = lines + 1;
		}

		output = "";
		if (is_file) {
			output.append("---------- ");
			output.append(rest);
			output.append(": ");
		}
		
		output.append(std::to_string(lines));
		output.append("\n");
		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
	}
	else {
		output = "Wrong arguments.\n";
		uint16_t exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		kiv_os_rtl::Exit(exit_code);
		return 0;
	}	
	int16_t exit_code = static_cast<uint16_t>(kiv_os::NOS_Error::Success);
	kiv_os_rtl::Exit(exit_code);
	return 0;
}