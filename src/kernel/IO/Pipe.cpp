#include "Pipe.h"

Pipe::Pipe(const size_t buffer_size) {
	buffer.reserve(buffer_size);
	write = std::make_shared<Semaphore>(buffer_size);
	read = std::make_shared<Semaphore>(0);
}

bool Pipe::Empty() const {
	return items == 0;
}

kiv_os::NOS_Error Pipe::Read(char* target_buffer, const size_t buffer_size, size_t& bytes_read) {
	auto bytes_read_from_buffer = 0;
	for (size_t i = 0; i < buffer_size; i += 1) {
		// Nejprve checkneme bez semaforu
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished && Empty()) {
				bytes_read = bytes_read_from_buffer;
				return kiv_os::NOS_Error::Success;
			}
		}

		// Ziskame semafor pro cteni
		read->Acquire();
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished && Empty()) {
				bytes_read = bytes_read_from_buffer;
				write->Release();
				return kiv_os::NOS_Error::Success;
			}
		}

		// Lockneme pristup k bufferu a precteme z nej
		{
			auto lock = std::scoped_lock(buffer_access);
			target_buffer[i] = buffer[read_idx];
			AdvanceReadIdx(); // Posuneme index pro cteni
			bytes_read_from_buffer += 1;
			items -= 1;
		}

		// Signalizujeme semafor pro zapis
		write->Release();
	}
	bytes_read = bytes_read_from_buffer;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Pipe::Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written) {
	size_t bytes_written_to_buffer = 0;
	for (size_t i = 0; i < buffer_size; i += 1) {
		// Nejprve check bez semaforu
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished) {
				bytes_written = bytes_written_to_buffer;
				return kiv_os::NOS_Error::Success;
			}
		}

		// Ziskame semafor pro zapis
		write->Acquire();
		{
			auto lock = std::scoped_lock(flag_access);
			if (read_finished || write_finished) {
				bytes_written = bytes_written_to_buffer;
				return kiv_os::NOS_Error::Success;
			}
		}
		// Lockneme pristup k bufferu a zapiseme do nej
		{
			auto lock = std::scoped_lock(buffer_access);
			buffer[write_idx] = source_buffer[i];
			AdvanceWriteIdx();
			items += 1;
			bytes_written_to_buffer += 1;
		}

		// Signalizujeme semafor pro cteni
		read->Release();
	}
	bytes_written = bytes_written_to_buffer;
	return kiv_os::NOS_Error::Success;
}

void Pipe::CloseWriting() {
	const char eot = static_cast<char>(kiv_hal::NControl_Codes::SUB);
	size_t bytes_written = 0;
	Write(&eot, 1, bytes_written);
	auto lock = std::scoped_lock(flag_access);
	write_finished = true;
}

void Pipe::CloseReading() {
	auto lock = std::scoped_lock(flag_access);
	read_finished = true;
}
