#pragma once
#include "AbstractFile.h"
#include "AbstractFile.h"

/// <summary>
/// Zapis do konzole
/// </summary>
class ConsoleOut final : public AbstractFile {
public:
	kiv_os::NOS_Error write(const char* sourceBuffer, size_t bytes, size_t& bytesWritten) override;
};
