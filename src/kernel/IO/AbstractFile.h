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
	virtual kiv_os::NOS_Error read(char* targetBuffer, uint32_t bytes, uint32_t& bytesRead) {
		return kiv_os::NOS_Error::Success;
	}
	virtual kiv_os::NOS_Error write(const char* sourceBuffer, uint32_t bufferSize, uint32_t& bytesWritten) {
		return kiv_os::NOS_Error::Success;
	}
	
};
