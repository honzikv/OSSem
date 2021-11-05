#pragma once
#include <string>
#include <unordered_map>
#include "../api/api.h"

/// <summary>
/// Stav procesu
/// </summary>
enum class ProcessState: uint8_t {
	Ready,
	Running,
	Stopped
};

/// <summary>
/// Reprezentuje proces (resp. PCB)
/// </summary>
struct Process {

	/// <summary>
	/// Stav procesu
	/// </summary>
	ProcessState processState = ProcessState::Ready;

	/// <summary>
	/// PID
	/// </summary>
	kiv_os::THandle processId;

	/// <summary>
	/// PID rodice
	/// </summary>
	kiv_os::THandle parentId;

	/// <summary>
	/// Standardni vstup
	/// </summary>
	kiv_os::THandle stdIn;

	/// <summary>
	/// Standardni vystup
	/// </summary>
	kiv_os::THandle stdOut;

	/// <summary>
	/// Slozka, ve ktere proces aktualne operuje
	/// </summary>
	std::string workDirectory;

	/// <summary>
	/// Mapping signalu a callbacku, ktery se provede po zavolani
	/// </summary>
	std::unordered_map<kiv_os::NSignal_Id, kiv_os::TThread_Proc> signalCallbacks;

	/// <summary>
	/// Navratovy kod procesu
	/// </summary>
	kiv_os::NOS_Error returnCode = kiv_os::NOS_Error::Success;
};

