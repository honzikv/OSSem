#pragma once
#include "AbstractFile.h"
#include "AbstractFile.h"

/// <summary>
/// Cteni z konzole
/// </summary>
class ConsoleIn final : public AbstractFile  {

public:
	kiv_os::NOS_Error Read(char* target_buffer, size_t bytes, size_t& bytes_read) override;
};
