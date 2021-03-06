#pragma once
#include <string>
#include "../../api/api.h"
#include "../Process/Process.h"

/// <summary>
/// Jedna radka pro snapshot tabulky
/// </summary>
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
	const uint8_t state;
	
	ProcFSRow(const std::string& program_name, const uint32_t running_threads, const kiv_os::THandle pid, const TaskState task_state);

	static std::shared_ptr<ProcFSRow> Map_Process_To_ProcFSRow(const Process& process);
	
};
