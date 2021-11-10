#pragma once
#include "IFile.h"

/// <summary>
/// Zapis do konzole
/// </summary>
class ConsoleOut final : public IFile {
public:
	kiv_os::NOS_Error Write(const char* source_buffer, size_t bytes, size_t& bytes_written) override;
};
