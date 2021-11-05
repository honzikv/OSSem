#pragma once
#include "AbstractFile.h"
#include "AbstractFile.h"

/// <summary>
/// Cteni z konzole
/// </summary>
class ConsoleIn final : public AbstractFile  {

public:
	kiv_os::NOS_Error read(char* targetBuffer, uint32_t bytes, uint32_t& bytesRead) override;
};
