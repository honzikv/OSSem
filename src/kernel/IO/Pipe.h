#pragma once
#include <vector>

#include "Utils/Semaphore.h"

class Pipe {

public:
	/// <summary>
	/// Defaultni velikost pro buffer
	/// </summary>
	static constexpr size_t DEFAULT_PIPE_BUFFER_SIZE = 1024; // 1 K
private:

	/// <summary>
	/// Semafor pro synchronizaci prazdnych polozek
	/// </summary>
	std::shared_ptr<Semaphore> empty;

	/// <summary>
	/// Semafor pro synchronizaci plnych polozek
	/// </summary>
	std::shared_ptr<Semaphore> full;

	/// <summary>
	/// Mutex pro thread-safe pristup k bufferu
	/// </summary>
	std::mutex array_access;

	/// <summary>
	/// Buffer pro data
	/// </summary>
	std::vector<uint8_t> byte_buffer;

	/// <summary>
	/// Zda-li se da z pipe cist
	/// </summary>
	std::atomic<bool> is_readable = { true };

	/// <summary>
	/// Zda-li se da to pipe zapisovat
	/// </summary>
	std::atomic<bool> is_writable = { true };

	Pipe(size_t buffer_size = DEFAULT_PIPE_BUFFER_SIZE) {
		byte_buffer.reserve(buffer_size);
	}
};

