#pragma once

#include "../../api/api.h"
#include "../rtl.h"
#include <array>
#include <iterator>


constexpr auto BufferSize = 1024;


// Pravdepodobne cistejsi by bylo toto napsat do api.h ...
/// <summary>
/// Stav procesu/vlakna
/// </summary>
enum class TaskState : uint8_t {
	// Pocatecni stav ve CreateThread / CreateProcess, kdy vlakno je vytvorene, ale jeste nebylo spusteno
	Ready,

	// Vlakno bezi a vykonava program
	Running,

	// Uloha byla dokoncena
	Finished,

};

struct ProcFSRow {

	/// <summary>
	/// Jmeno programu
	/// </summary>
	const std::string program_name;

	/// <summary>
	/// Pocet vlaken
	/// </summary>
	const uint32_t running_threads;

	/// <summary>
	/// Pid procesu
	/// </summary>
	const kiv_os::THandle pid;

	/// <summary>
	/// Stav procesu
	/// </summary>
	const TaskState state;

	ProcFSRow(std::string program_name, uint32_t running_threads, kiv_os::THandle pid, TaskState task_state);

	std::string Get_State_Str() const;

	std::string To_String() const;
};


std::string Read_Process_Name(const char* buffer, size_t start_idx, size_t buffer_size, size_t& return_idx);

extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters& regs);
