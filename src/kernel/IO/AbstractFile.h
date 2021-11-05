#pragma once
#include <cstdint>
#include <vector>

#include "../api/api.h"

/// <summary>
/// Abstraktni trida, ktera reprezentuje "soubor" pro io.
///
///	Zde tuto tridu implementuje vsechno co pouziva THandle a ma metody pro psani / zapis
/// </summary>
class AbstractFile {
public:
	virtual ~AbstractFile() = default;
	virtual kiv_os::NOS_Error read(char* targetBuffer, size_t bytes, size_t& bytesRead) {
		return kiv_os::NOS_Error::Success;
	}
	virtual kiv_os::NOS_Error write(const char* sourceBuffer, size_t bufferSize, size_t& bytesWritten) {
		return kiv_os::NOS_Error::Success;
	}
	
};
