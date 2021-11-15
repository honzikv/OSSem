#include "Pipe.h"

#include "Utils/Logging.h"

Pipe::Pipe(const size_t buffer_size): write(std::make_shared<Semaphore>(buffer_size)),
                                      read(std::make_shared<Semaphore>()) {
	buffer.reserve(buffer_size);
}

bool Pipe::Empty() const {
	return items == 0;
}



kiv_os::NOS_Error Pipe::Read(char* target_buffer, const size_t buffer_size, size_t& bytes_read) {
	auto bytes_read_from_buffer = 0;
	LogDebug("Pipe read start");
	for (size_t i = 0; i < buffer_size; i += 1) {
		// Nejprve checkneme bez semaforu
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished && Empty()) {
				bytes_read = bytes_read_from_buffer;
				return kiv_os::NOS_Error::Permission_Denied;
			}
		}

		// Ziskame semafor pro cteni
		read->Acquire();
		LogDebug("Read lock acquired");
		// Lockneme pristup k bufferu a flagum a precteme z nej
		{
			auto lock = std::scoped_lock(buffer_access, flag_access);
			if (read_finished || write_finished && Empty()) {
				bytes_read = bytes_read_from_buffer;
				write->Release();
				return kiv_os::NOS_Error::Permission_Denied;
			}

			// Pokud jsme precetli EOF skoncime cteni
			target_buffer[i] = buffer[read_idx];
			if (target_buffer[i] == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
				read_finished = true;
			}

			AdvanceReadIdx(); // Posuneme index pro cteni
			bytes_read_from_buffer += 1;
			items -= 1;
			LogDebug("Reading an item finished. Remaining items to read: " + std::to_string(items));
		}

		// Signalizujeme semafor pro zapis
		write->Release();
	}
	bytes_read = bytes_read_from_buffer;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Pipe::Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) {
	size_t bytes_written_to_buffer = 0;
	LogDebug("Pipe write start");
	for (size_t i = 0; i < buffer_size; i += 1) {
		// Nejprve check bez semaforu
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished) {
				bytes_written = bytes_written_to_buffer;
				return kiv_os::NOS_Error::Permission_Denied;
			}
		}

		// Ziskame semafor pro zapis
		write->Acquire();
		LogDebug("Writing lock acquired");
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished) {
				bytes_written = bytes_written_to_buffer;
				return kiv_os::NOS_Error::Permission_Denied;
			}
		}
		// Lockneme pristup k bufferu a zapiseme do nej
		{
			auto lock = std::scoped_lock(buffer_access);
			buffer[write_idx] = source_buffer[i];
			AdvanceWriteIdx();
			items += 1;
			bytes_written_to_buffer += 1;
			LogDebug("Writing an item finished. Remaining items to read: " + std::to_string(items));
		}

		// Signalizujeme semafor pro cteni
		read->Release();
	}
	bytes_written = bytes_written_to_buffer;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Pipe::Close() {
	constexpr char eof = static_cast<char>(kiv_hal::NControl_Codes::SUB);
	size_t bytes_written = 0;
	Write(&eof, 1, bytes_written);
	auto lock = std::scoped_lock(flag_access);
	write_finished = true;
	read->Release();

	return kiv_os::NOS_Error::Success;
}
