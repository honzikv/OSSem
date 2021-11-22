#pragma once
#include <cstdint>

/// <summary>
/// Stav procesu/vlakna
/// </summary>
enum class TaskState : uint8_t {
	// Pocatecni stav ve CreateThread / CreateProcess, kdy vlakno je vytvorene, ale jeste nebylo spusteno
	Ready,

	// Vlakno bezi a vykonava program
	Running,

	// ProgramFinished je stav, kdy program dobehl uspesne a jeste se nezavolali callbacky
	ProgramFinished,

	// ProgramTerminated je stav, kdy program nedobehl a jeste se nezavolali callbacky
	ProgramTerminated,

	// Readable exit code je stav po zavolani callbacku pri stavu finished a terminated
	ReadableExitCode
};
