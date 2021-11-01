#pragma once

#include <string>
#include <vector>

#include "../api/api.h"


enum class ProcessState {
	Running,
	Waiting,
	Finished
};

/**
 * Obsahuje data pro procesy
 */
class ProcessControlBlock {

	/*
	 * Process ID
	 * Unikatni identifikator pro proces v tabulce procesu
	 */
	kiv_os::THandle pid;

	/**
	 * PID rodice
	 */
	kiv_os::THandle parentPid;

	/**
	 * Jmeno programu procesu
	 */
	const std::string program;

	/**
	 * 
	 */
	std::string workingDirectory;

	/**
	 * Fronta cekajicich procesu
	 */
	std::vector<kiv_os::THandle> waitingProcesses;
};
