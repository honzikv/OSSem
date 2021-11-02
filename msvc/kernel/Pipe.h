#pragma once
#include <atomic>
#include <vector>

#include "AbstractFile.h"

/**
 * Struktura pro komunikaci mezi dvema procesy
 */
class Pipe : AbstractFile {

	uint32_t bufferSize;
	
	std::vector<char> buffer;

	/**
	 * Boolean pro zavreni pipy
	 */
	std::atomic<bool> isPipeClosed = false;

public:
	kiv_os::NOS_Error write(const std::vector<char>& buffer) override;
	kiv_os::NOS_Error read(uint32_t bytes) override;
	kiv_os::NOS_Error moveTo(uint32_t position) override;
	kiv_os::NOS_Error close() override {
		isPipeClosed = true;
		return kiv_os::NOS_Error::Success;
	}
};

