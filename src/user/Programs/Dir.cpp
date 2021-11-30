#include "Dir.h"

#include <string>
#include <vector>

#include "rtl.h"

extern "C" size_t __stdcall dir(const kiv_hal::TRegisters & regs) {

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	auto args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	size_t written;
	size_t read;

	size_t current_index = 0;
	size_t index = 0;
	int dir_count = 0;
	int file_count = 0;
	constexpr int max_item_count = 10;
	constexpr size_t dir_entry_size = sizeof(kiv_os::TDir_Entry);
	constexpr int char_buffer_size = max_item_count * dir_entry_size;

	// pokud neni zadana cesta, pouzije se aktualni adresar
	/*if (args.empty()) {
		args = std::string(".");
	}*/

	auto buffer = std::array<char, char_buffer_size>();
	kiv_os::THandle handle;
	std::string output;
	if (!kiv_os_rtl::Open_File(handle, args, kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Directory)) {

		// TODO smazat
		std::string tmp2("\n Chyba otevreni souboru\n");
		kiv_os_rtl::Write_File(std_out, tmp2.data(), tmp2.size(), written);
		// TODO konec smazat


		kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found));
		return 1;
	}

	kiv_os_rtl::Read_File(handle, buffer.data(), char_buffer_size, read);

	while (read) {
		while (current_index < (read / sizeof(kiv_os::TDir_Entry))) {

			const auto entry = reinterpret_cast<kiv_os::TDir_Entry*>(buffer.data() + current_index * dir_entry_size);
			if (entry->file_attributes == static_cast<uint16_t>(kiv_os::NFile_Attributes::Directory)) {
				output.append("<DIR>");
				dir_count++;
			}
			else {
				file_count++;
			}

			output.append("\t\t");
			output.append(entry->file_name);
			output.append("\n");
			current_index++;
		}
		if (read < char_buffer_size) {
			// uz tam nic dalsiho neni
			break;
		}
		index += current_index;
		current_index = 0;

		// TODO nemelo by byt potreba
		//// Set seek of directory to index which has not been yet processed.
		//kiv_os_rtl::Seek(handle, kiv_os::NFile_Seek::Set_Position, kiv_os::NFile_Seek::Beginning, index);

		kiv_os_rtl::Read_File(handle, buffer.data(), char_buffer_size, read);
	}

	output.append("\n");
	output.append("File(s): ");
	output.append(std::to_string(file_count));
	output.append("\n");

	output.append("Dir(s): ");
	output.append(std::to_string(dir_count));
	output.append("\n");

	kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
	kiv_os_rtl::Close_File_Descriptor(handle);
	
	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}
