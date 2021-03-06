#include "ProcessTableSnapshot.h"
#include "../Utils/ByteUtils.h"

#include <iterator>
#include <sstream>

ProcessTableSnapshot::ProcessTableSnapshot(const std::vector<std::shared_ptr<ProcFSRow>> procfs_rows) {
	auto byte_stream = std::stringstream();

	for (const auto& procfs_row : procfs_rows) {
		ByteUtils::Copy_To_String_Stream(procfs_row->pid, byte_stream);

		// char muzeme nakopirovat rucne
		auto process_name = procfs_row->program_name;
		auto max_idx = process_name.size() > MaxProcessNameLen ? MaxProcessNameLen : process_name.size();
		for (size_t i = 0; i < max_idx; i += 1) {
			byte_stream << procfs_row->program_name.at(i);
		}

		// aby slo string spravne precist musime pridat null-terminaci
		ByteUtils::Copy_To_String_Stream('\0', byte_stream);

		ByteUtils::Copy_To_String_Stream(procfs_row->running_threads, byte_stream);
		ByteUtils::Copy_To_String_Stream(procfs_row->state, byte_stream);
	}

	// Byty ve stringu
	const auto str_bytes = byte_stream.str();

	// Nakopirujeme do bufferu
	std::copy_n(str_bytes.c_str(), str_bytes.size(), std::back_inserter(bytes));
}

kiv_os::NOS_Error ProcessTableSnapshot::Read(char* target_buffer, const size_t buffer_size, size_t& bytes_read) {
	auto bytes_read_from_buffer = 0;
	for (size_t i = 0; i < buffer_size; i += 1) {
		if (idx >= bytes.size()) {
			bytes_read = bytes_read_from_buffer;
			return kiv_os::NOS_Error::Success;
		}

		target_buffer[i] = bytes[idx];
		idx += 1; // pretocime index o 1
		bytes_read_from_buffer += 1; // zvysime pocet prectenych bytu
	}

	return kiv_os::NOS_Error::Success; // success
}

kiv_os::NOS_Error ProcessTableSnapshot::Close() {
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessTableSnapshot::Seek(const size_t position, const kiv_os::NFile_Seek seek_type,
                                             const kiv_os::NFile_Seek seek_operation, size_t& res_pos) {
	switch (seek_operation) {
		case kiv_os::NFile_Seek::Set_Size: {
			return kiv_os::NOS_Error::Permission_Denied;
		}
		case kiv_os::NFile_Seek::Get_Position:
			res_pos = idx;
			break;
		default:
		case kiv_os::NFile_Seek::Set_Position:
			const auto file_size = bytes.size();
			switch (seek_type) {
				default:
				case kiv_os::NFile_Seek::Current:
					// z nejakeho duvodu nefungovalo std::max macro
					idx = idx + position > file_size ? file_size : idx + position;
					break;
				case kiv_os::NFile_Seek::Beginning:
					idx = position; // jen na danou pozici
					break;
				case kiv_os::NFile_Seek::End:
					idx = file_size - position < 0 ? 0 : file_size - position; // od konce
					break;
			}
			res_pos = idx;
	}
	return kiv_os::NOS_Error::Success;
}
