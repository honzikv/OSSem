#pragma once
#include <atomic>
#include <vector>

#include "AbstractFile.h"

/**
 * Struktura pro komunikaci mezi dvema procesy
 */
class Pipe : public AbstractFile {

	uint32_t bufferSize;
	
	std::vector<char> buffer;

	/**
	 * Boolean pro zavreni pipy
	 */
	std::atomic<bool> isPipeClosed = false;
	
};

