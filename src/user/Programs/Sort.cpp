#include "Sort.h"

size_t __stdcall sort(const kiv_hal::TRegisters &regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	size_t written;
	std::string output;

	kiv_os::THandle handler_in = std_in;
	bool is_file = false;
	if (!args.empty()) {
		size_t actual_position = 0;
		if (!kiv_os_rtl::Open_File(handler_in, args, kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::System_File)) {
			auto err_message = std::string("Can not open file ");
			err_message.append(args);
			err_message.append(".\n");
			kiv_os_rtl::Write_File(std_out, err_message.data(), err_message.size(), written);

			kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found));
			return 1;
		}

		
		is_file = true;
		kiv_os_rtl::Seek(handler_in, kiv_os::NFile_Seek::Set_Position, kiv_os::NFile_Seek::Beginning, actual_position);
	}

	constexpr int buffer_size = 256;
	size_t read = 1;
	std::vector<char> read_buffer(buffer_size);
	std::string input_data;

	while (read) {
		if (!kiv_os_rtl::Read_File(handler_in, read_buffer.data(), buffer_size, read)) {
			// uz tam nejspis nic neni
			break;
		}
		//pokud je posledni precteny znak konec souboru nebo ukonceni vstupu (CTRL+Z)
		if (read_buffer[read - 1] == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
			break;
		}
		input_data.append(read_buffer.data(), 0, read);
	}

	if (is_file) {
		kiv_os_rtl::Close_File_Descriptor(handler_in);
	}

	std::vector<std::string> lines;
	std::string line;

	std::stringstream string_stream(input_data);

	// rozdeli vstup na jednotlive radky
	while (std::getline(string_stream, line, '\n')) {
		lines.push_back(line);
	}

	std::sort(lines.begin(), lines.end());

	input_data.clear();

	for (const auto& it : lines) {
		output.append(it);
		output.append("\n");
	}
	
	kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
	
	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}