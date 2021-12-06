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

	// Vlakno ceka, (tento stav se nakonec nepouzil)
	Waiting,

	// Uloha byla dokoncena
	Finished,
	
};
