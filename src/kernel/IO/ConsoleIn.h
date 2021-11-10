#pragma once
#include "IFile.h"

/// <summary>
/// Cteni z konzole
/// </summary>
class ConsoleIn final : public IFile  {

public:
	kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read) override;
};
