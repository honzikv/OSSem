#pragma once
#include <string>

#include "../api/api.h"

enum class ProcessState: uint8_t {
	Ready,
	Running,
	Stopped
};

/**
 * Reprezentuje kontext processu (PCB)
 */
class Process {

	ProcessState processState = ProcessState::Ready;

	/**
	 * pid
	 */
	kiv_os::THandle processId;

	/**
	 * Id pidu rodice
	 */
	kiv_os::THandle parentId;

	/**
	 * Standardni vstup
	 */
	kiv_os::THandle stdIn;

	/**
	 * Standardni vystup
	 */
	kiv_os::THandle stdOut;

	/**
	 * Kde na disku se aktualne proces nachazi
	 */
	std::string workDirectory;

};

