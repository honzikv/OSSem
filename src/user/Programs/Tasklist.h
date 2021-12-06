#pragma once

#include "../../api/api.h"
#include "../rtl.h"

/// <summary>
/// Velikost bufferu
/// </summary>
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

/// <summary>
/// Struktura pro ulozeni jednoho radku tabulky
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
	const TaskState state;

	/// <summary>
	/// Konstruktor
	/// </summary>
	/// <param name="program_name">Jmeno programu</param>
	/// <param name="running_threads">Pocet bezicich vlaken</param>
	/// <param name="pid">pid</param>
	/// <param name="task_state">stav</param>
	ProcFSRow(std::string program_name, uint32_t running_threads, kiv_os::THandle pid, TaskState task_state);

	/// <summary>
	/// Vrati State jako string
	/// </summary>
	/// <returns></returns>
	std::string Get_State_Str() const;

	/// <summary>
	/// Vrati ToString pro vytisknuti do standardniho vystupu
	/// </summary>
	///	<param name="name_len">Delka nazvu bez mezery (pro zarovnani)</param>
	/// <returns></returns>
	std::string To_String(size_t name_len) const;
};

/// <summary>
/// Precte jmeno procesu
/// </summary>
/// <param name="buffer"></param>
/// <param name="start_idx"></param>
/// <param name="buffer_size"></param>
/// <param name="return_idx"></param>
/// <returns></returns>
std::string Read_Process_Name(const char* buffer, size_t start_idx, size_t buffer_size, size_t& return_idx);

extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters& regs);
