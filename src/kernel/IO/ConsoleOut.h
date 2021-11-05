#pragma once
#include "AbstractFile.h"
#include "AbstractFile.h"

class ConsoleOut final : public AbstractFile {
public:
	kiv_os::NOS_Error write(const char* sourceBuffer, size_t bufferSize, size_t& bytesWritten) override;
};
