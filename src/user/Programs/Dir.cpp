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
	bool is_recursive = false;
	constexpr auto recursion_param = "/s";

	size_t current_index = 0;
	size_t index = 0;
	int dir_count = 0;
	int file_count = 0;
	constexpr int max_item_count = 10;
	constexpr size_t dir_entry_size = sizeof(kiv_os::TDir_Entry);
	constexpr int char_buffer_size = max_item_count * dir_entry_size;

	std::string dir_name;
	if (!args.empty()) {
		// use whitespace as a separator
		constexpr auto separator = " ";

		if (const auto separator_index = args.find(separator);
			separator_index > 0 && separator_index != std::string::npos) {
			if (recursion_param == args.substr(0, separator_index)) {
				// zapnout rekurzi
				is_recursive = true;
				dir_name = args.substr(separator_index + 1);
			}
			else {
				// je argument, ktery ale neni podporovany
				const std::string err_message("Invalid first argument. The only supported argument is /s.\n");
				kiv_os_rtl::Write_File(std_out, err_message.data(), err_message.size(), written);
				kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument));
				return 1;
			}
		}
		else if (args == recursion_param) {
			is_recursive = true;
			dir_name = "";
		}
		else {
			dir_name = args;
		}
	}
	

	auto buffer = std::array<char, char_buffer_size>();
	kiv_os::THandle handle;
	std::string output;
	std::vector<std::string> dirs_to_process{};
	dirs_to_process.push_back(dir_name);

	std::string actual_path;
	

	size_t file_start_index = 0;

	while (!dirs_to_process.empty()) {
		const auto entry = dirs_to_process.back();
		// vymaze posledni prvek
		dirs_to_process.pop_back();

		// slozky, ktere se pridaji do hlavniho vektoru k prochazeni
		std::vector<std::string> actual_dirs;
		actual_dirs.reserve(5);

		if (!kiv_os_rtl::Open_File(handle, entry, kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Directory)) {

			std::string err_message("Could not open directory.\n");
			kiv_os_rtl::Write_File(std_out, err_message.data(), err_message.size(), written);

			kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found));
			return 1;
		}
		// nastaveni na zacatek souboru
		kiv_os_rtl::Seek(handle, kiv_os::NFile_Seek::Set_Position, kiv_os::NFile_Seek::Beginning, file_start_index);

		if (is_recursive) {
			output.append(" Directory of: ");
			output.append(entry);
			output.append("\n\n");
		}
		read = 1;
		while (read) {
			file_count = 0;
			dir_count = 0;
			current_index = 0;

			if (!kiv_os_rtl::Read_File(handle, buffer.data(), char_buffer_size, read)) {
				// nemuzu precist file
				break;
			}

			while (current_index < (read / sizeof(kiv_os::TDir_Entry))) {

				auto curr_entry = reinterpret_cast<kiv_os::TDir_Entry*>(buffer.data() + current_index * dir_entry_size);
				
				if (curr_entry->file_attributes == static_cast<uint16_t>(kiv_os::NFile_Attributes::Directory)) {
					output.append("<DIR>");
					dir_count++;
					if (is_recursive) {
						if (strlen(curr_entry->file_name) > 0) {
							std::string tmp_dir_path = entry + "\\" + curr_entry->file_name;
							actual_dirs.push_back(tmp_dir_path);
						}
					}
				}
				else {
					file_count++;
				}

				output.append("\t\t");
				output.append(curr_entry->file_name);
				output.append("\n");
				current_index++;
			}
			if (read < char_buffer_size) {
				// uz tam nic dalsiho neni
				current_index = 0;
				break;
			}
			index += current_index;
			current_index = 0;
		}

		kiv_os_rtl::Close_File_Descriptor(handle);

		output.append("\n");
		output.append("File(s): ");
		output.append(std::to_string(file_count));
		output.append("\n");

		output.append("Dir(s): ");
		output.append(std::to_string(dir_count));
		output.append("\n\n");

		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		output.clear();

		if (is_recursive) {
			// presune zbyvajici adresare do hlavniho vectoru tak, aby se zpracovavali ve spravnem poradi
			while (!actual_dirs.empty()) {
				dirs_to_process.push_back(actual_dirs.back());
				actual_dirs.pop_back();
			}
		}
	}

	
	kiv_os_rtl::Exit(static_cast<uint16_t>(kiv_os::NOS_Error::Success));
	return 0;
}
