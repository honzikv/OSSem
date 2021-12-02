#include "Tasklist.h"
#include <sstream>
#include "Utils/StringUtils.h"
#include "Utils/Logging.h"

ProcFSRow::ProcFSRow(std::string program_name, const uint32_t running_threads, const kiv_os::THandle pid,
                     const TaskState task_state): program_name(std::move(program_name)),
                                                  running_threads(running_threads), pid(pid),
                                                  state(task_state) { }

std::string ProcFSRow::Get_State_Str() const {
	switch (state) {
		case TaskState::Finished:
			return "finished";
		case TaskState::Ready:
			return "ready";
		case TaskState::Running:
		default:
			return "running";
	}
}

std::string ProcFSRow::To_String() const {
	auto string_stream = std::stringstream();
	string_stream << std::to_string(pid) << "\t" << program_name << "\t\t" << Get_State_Str() << "\t" << std::to_string(running_threads) << "\n";
	return string_stream.str();
}

std::string Read_Process_Name(const char* buffer, const size_t start_idx, const size_t buffer_size, size_t& return_idx) {
	auto result = std::vector<char>();
	auto return_idx_set = false;

	for (size_t i = start_idx; i < buffer_size; i += 1) {
		const auto symbol = buffer[i];
		if (symbol == '\0') {
			// pokud jsme nasli \0 vratime se
			return_idx_set = true;
			return_idx = i + 1;
			break;
		}

		result.push_back(symbol);
	}

	if (!return_idx_set) {
		return_idx = buffer_size; // jsme na konci
	}

	return std::string(result.begin(), result.end());
}

size_t tasklist(const kiv_hal::TRegisters& regs) {
	const auto ProcfsFilePath = "p:\\proclst";
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	auto procfs_file_descriptor = kiv_os::Invalid_Handle;
	const auto success = kiv_os_rtl::Open_File(procfs_file_descriptor, ProcfsFilePath,
	                                           kiv_os::NOpen_File::fmOpen_Always,
	                                           kiv_os::NFile_Attributes::System_File);

	if (!success) {
		const auto err_message = std::string("Error, OS did not create a tasklist file to read from.\n");
		size_t written = 0;
		kiv_os_rtl::Write_File(std_out, err_message.c_str(), err_message.size(), written);
		return 1; // Chyba otevreni souboru
	}

	// Jinak zacneme cist ze souboru
	// Jedna se o jednoduchy buffer, kde je vzdy zapsan pid procesu (16 bitu), string ukonceny \0, pocet bezicich vlaken (32 bitu) a
	// stav (8 bitu)
	auto buffer = std::array<char, BufferSize>(); // budeme po 1k dat
	size_t bytes_read = 0;

	// Data se budou ukladat do vektoru. Protoze je tabulka mala, nemusime to resit jinak
	auto data = std::vector<char>();

	while (kiv_os_rtl::Read_File(procfs_file_descriptor, buffer.data(), BufferSize, bytes_read)) {
		if (bytes_read == 0) {
			break;
		}

		// Vlozime data na konec bufferu
		std::copy_n(buffer.begin(), bytes_read, std::back_inserter(data));
	}

	// vektor namapovanych procfs radek
	auto procfs_rows = std::vector<ProcFSRow>();

	// Nyni staci interpretovat data v bufferu
	size_t idx = 0;
	while (idx < data.size()) {
		// velmi sofistikovana iterace
		// Precteme pid
		kiv_os::THandle pid;
		std::memcpy(std::addressof(pid), std::addressof(data[idx]), sizeof(kiv_os::THandle));
		idx += sizeof(kiv_os::THandle);

		// Index se rovnou premapuje ve funkci
		auto program_name = Read_Process_Name(data.data(), idx, data.size(), idx);

		// Pocet bezicich vlaken
		uint32_t running_threads;
		std::memcpy(std::addressof(running_threads), std::addressof(data[idx]), sizeof(uint32_t));
		idx += sizeof(uint32_t);

		uint8_t state_b;
		std::memcpy(std::addressof(state_b), std::addressof(data[idx]), sizeof(uint8_t));
		idx += sizeof(uint8_t);
		auto state = static_cast<TaskState>(state_b);

		// Pridame do vektoru
		procfs_rows.emplace_back(program_name, running_threads, pid, state);
	}

	for (const auto& procfs_row : procfs_rows) {
		auto procfs_str = procfs_row.To_String();
		size_t written = 0;
		const auto success = kiv_os_rtl::Write_File(std_out, procfs_str.data(), procfs_str.size(), written);
		Log_Debug("success: " + std::to_string(success) + " error?: " + StringUtils::Err_To_String(kiv_os_rtl::Get_Last_Err()));
	}

	kiv_os_rtl::Close_File_Descriptor(procfs_file_descriptor);

	return 0;
}
