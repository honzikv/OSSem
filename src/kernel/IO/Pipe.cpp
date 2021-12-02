#include "Pipe.h"

#include "../Utils/Logging.h"

Pipe::Pipe(const size_t buffer_size): write(std::make_unique<Semaphore>(buffer_size)),
                                      read(std::make_unique<Semaphore>()) {
	buffer.resize(buffer_size);
	Log_Debug("New Pipe with buffer size: " + std::to_string(buffer.size()));
}

bool Pipe::Empty() const {
	return items == 0;
}

bool Pipe::Full() const {
	return items == buffer.size();
}

void Pipe::Advance_Reading_Idx() {
	// LogDebug("Reading index moved from: " + std::to_string(reading_idx) + " to: " + std::to_string((reading_idx + 1 )% buffer.size()));
	reading_idx = (reading_idx + 1) % buffer.size();
}

void Pipe::Advance_Writing_Idx() {
	// LogDebug("Writing index moved from: " + std::to_string(writing_idx) + " to: " + std::to_string((writing_idx + 1) % buffer.size()));
	writing_idx = (writing_idx + 1) % buffer.size();
}

kiv_os::NOS_Error Pipe::Read(char* target_buffer, const size_t buffer_size, size_t& bytes_read) {
	auto bytes_read_from_buffer = 0;

	for (size_t i = 0; i < buffer_size; i += 1) {
		{
			auto lock = std::scoped_lock(close_access, pipe_access);
			if (writing_closed && Empty() || reading_closed) {
				target_buffer[i] = static_cast<char>(kiv_hal::NControl_Codes::SUB); // vratime EOF
				bytes_read_from_buffer += 1;
				bytes_read = bytes_read_from_buffer;
				return kiv_os::NOS_Error::IO_Error;
			}
		}

		// Ziskame semafor pro cteni
		read->Acquire();
		// Zkontrolujeme zda-li lze cist
		{
			auto lock = std::scoped_lock(close_access, pipe_access);
			if (writing_closed && Empty() || reading_closed) {
				target_buffer[i] = static_cast<char>(kiv_hal::NControl_Codes::SUB); // vratime EOF
				bytes_read_from_buffer += 1;
				bytes_read = bytes_read_from_buffer;
				read->Release(); // aby nedoslo k deadlocku
				return kiv_os::NOS_Error::IO_Error;
			}
		}

		// lockeneme pristup k bufferu
		auto lock = std::scoped_lock(pipe_access);
		// Precteme symbol a pridame ho do bufferu
		const auto symbol = buffer[reading_idx];
		target_buffer[i] = symbol;
		bytes_read_from_buffer += 1;
		// Posuneme index pro cteni a snizime pocet polozek o 1
		Advance_Reading_Idx();
		items -= 1;
		write->Release();
	}
	bytes_read = bytes_read_from_buffer;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Pipe::Write(const char* source_buffer, const size_t buffer_size, size_t& bytes_written) {
	auto bytes_written_to_buffer = 0;
	for (size_t i = 0; i < buffer_size; i += 1) {
		{
			auto lock = std::scoped_lock(close_access);
			if (writing_closed || reading_closed) {
				bytes_written = bytes_written_to_buffer;
				return kiv_os::NOS_Error::Permission_Denied;
			}
		}

		write->Acquire();
		{
			auto lock = std::scoped_lock(close_access);
			if (writing_closed || reading_closed) {
				bytes_written = bytes_written_to_buffer;
				write->Release();
				read->Release();
				return kiv_os::NOS_Error::Permission_Denied;
			}
		}

		auto lock = std::scoped_lock(pipe_access);
		// Pridame prvek do bufferu
		const auto symbol = source_buffer[i];
		buffer[writing_idx] = symbol;
		bytes_written_to_buffer += 1;

		// Zvysime index pro zapis a pocet predmetu o 1
		Advance_Writing_Idx();
		items += 1;

		read->Release();
	}

	bytes_written = bytes_written_to_buffer;
	return kiv_os::NOS_Error::Success;
}


void Pipe::Close_For_Reading() {
	auto lock = std::scoped_lock(close_access);
	if (reading_closed) {
		return;
	}

	reading_closed = true;
	write->Release();
	read->Release();
	Log_Debug("Closing pipe for reading");
}

void Pipe::Close_For_Writing() {
	auto lock = std::scoped_lock(close_access);
	if (writing_closed) {
		return;
	}
	// Log_Debug("Trying to close pipe");
	// auto eof = static_cast<char>(kiv_hal::NControl_Codes::SUB);
	// auto bytes_written = size_t{0};
	// Write(std::addressof(eof), 1, bytes_written); // toto nastavi flag writing closed za nas

	writing_closed = true;
	write->Release(); // Vzbudime kohokoliv kdo chce zapisovat
	read->Release();
	Log_Debug("Closing pipe for writing");
}
