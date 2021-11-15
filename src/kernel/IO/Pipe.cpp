#include "Pipe.h"

#include "Utils/Logging.h"

Pipe::Pipe(const size_t buffer_size): write(std::make_shared<Semaphore>(buffer_size)),
                                      read(std::make_shared<Semaphore>()) {
	buffer.resize(buffer_size);
	LogDebug("New Pipe with buffer size: " + std::to_string(buffer.size()));
}

bool Pipe::Empty() const {
	return items == 0;
}

bool Pipe::Full() const {
	return items == buffer.size();
}

void Pipe::AdvanceReadingIdx() {
	LogDebug("Reading index moved from: " + std::to_string(reading_idx) + " to: " + std::to_string((reading_idx + 1 )% buffer.size()));
	reading_idx = (reading_idx + 1) % buffer.size();
}

void Pipe::AdvanceWritingIdx() {
	LogDebug("Writing index moved from: " + std::to_string(writing_idx) + " to: " + std::to_string((writing_idx + 1) % buffer.size()));
	writing_idx = (writing_idx + 1) % buffer.size();
}

kiv_os::NOS_Error Pipe::Read(char* target_buffer, const size_t buffer_size, size_t& bytes_read) {
	auto bytes_read_from_buffer = 0;
	{
		// Nejprve zkontrolujeme, jestli je co precist a pipe je pro cteni uzavrena
		auto lock = std::scoped_lock(pipe_access);
		if (writing_closed && items == 0 || reading_closed) {
			bytes_read = 0;
			return kiv_os::NOS_Error::Permission_Denied;
		}
	}

	// Pokud ne zacneme cist
	for (size_t i = 0; i < buffer_size; i += 1) {
		// Ziskame semafor pro cteni
		read->Acquire();

		// Zkusime zamknout pristup k bufferu a flagum
		auto lock = std::scoped_lock(pipe_access);
		if (writing_closed && items == 0 || reading_closed) {
			bytes_read = bytes_read_from_buffer;
			return kiv_os::NOS_Error::Success;
		}

		// Precteme symbol a pridame ho do bufferu
		const auto symbol = buffer[reading_idx];
		target_buffer[i] = symbol;
		bytes_read_from_buffer += 1;
		// Pokud byl symbol EOF zavreme pipe pro cteni
		if (symbol == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
			reading_closed = true;
			bytes_read = bytes_read_from_buffer;
			return kiv_os::NOS_Error::Success;
		}

		// Posuneme index pro cteni a snizime pocet polozek o 1
		AdvanceReadingIdx();
		items -= 1;
		// Notifikujeme cokoliv co je zablokovane na psani
		write->Release();
	}

	bytes_read = bytes_read_from_buffer;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Pipe::Write(const char* source_buffer, const size_t buffer_size, size_t& bytes_written) {
	auto bytes_written_to_buffer = 0;
	{
		auto lock = std::scoped_lock(pipe_access);
		if (writing_closed || reading_closed) {
			bytes_written = 0;
			return kiv_os::NOS_Error::Permission_Denied;
		}
	}

	for (size_t i = 0; i < buffer_size; i += 1) {
		// Ziskame semafor pro zapis
		write->Acquire();

		// Zkusime zamknout pristup k bufferu a flagum
		auto lock = std::scoped_lock(pipe_access);
		if (reading_closed || writing_closed) {
			bytes_written = bytes_written_to_buffer;
			return kiv_os::NOS_Error::Success;
		}

		// Pridame prvek do bufferu
		const auto symbol = source_buffer[i];
		buffer[writing_idx] = symbol;
		bytes_written_to_buffer += 1;

		// Pokud je prvek EOF ukoncime zapis
		if (symbol == static_cast<char>(kiv_hal::NControl_Codes::SUB)) {
			writing_closed = true;
			bytes_written = bytes_written_to_buffer;
			return kiv_os::NOS_Error::Success;
		}

		// Zvysime index pro zapis a pocet predmetu o 1
		AdvanceWritingIdx();
		items += 1;

		read->Release();
	}

	bytes_written = bytes_written_to_buffer;
	return kiv_os::NOS_Error::Success;
}


void Pipe::CloseForReading() {
	auto lock = std::scoped_lock(pipe_access);
	reading_closed = true;
	write->Release();
}

void Pipe::CloseForWriting() {
	auto eof = static_cast<char>(kiv_hal::NControl_Codes::SUB);
	auto _ = size_t{0};
	Write(std::addressof(eof), 1, _); // toto nastavi flag writing closed za nas
}
