#pragma once
#include "AbstractFile.h"
#include "AbstractFile.h"

class ConsoleOut final : public AbstractFile {
public:
	kiv_os::NOS_Error write(const char* sourceBuffer, uint32_t bufferSize, uint32_t& bytesWritten) override;
};
