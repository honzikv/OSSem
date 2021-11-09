#pragma once
#include <memory>
#include <unordered_set>
#include <array>
#include "../api/api.h"
#include "CommandParser.h"
#include "rtl.h"
#include "Utils/Logging.h"

constexpr auto NEWLINE_MESSAGE = "\n";
constexpr size_t BUFFER_SIZE = 512;
constexpr auto NEWLINE_SYMBOL = "\n";
constexpr auto EXIT_COMMAND = "exit";



extern "C" size_t __stdcall shell(const kiv_hal::TRegisters& regs);

/// <summary>
/// Pro prehlednost je shell objekt
/// </summary>
class Shell {

	/// <summary>
	/// Reference na registry
	/// </summary>
	kiv_hal::TRegisters registers;

	/// <summary>
	/// Reference na standardni vstup a vystup
	/// </summary>
	const kiv_os::THandle std_in, std_out;

	/// <summary>
	/// Instance command parseru
	/// </summary>
	CommandParser command_parser;

	/// <summary>
	/// Aktualni cesta, ve ktere se shell nachazi
	/// </summary>
	std::string current_path;

	/// <summary>
	/// Buffer na IO
	/// </summary>
	std::array<char, BUFFER_SIZE> buffer = {};

	bool exit_triggered = false;

	void Write(const std::string& message) const;

	void WriteLine(const std::string& message) const;


public:
	/// <summary>
	/// Konstruktor
	/// </summary>
	/// <param name="registers">Registry</param>
	/// <param name="std_in">Standardni vstup</param>
	/// <param name="std_out">Standardni vystup</param>
	/// <param name="current_path">Aktualni cesta</param>
	Shell(const kiv_hal::TRegisters& registers, kiv_os::THandle std_in, kiv_os::THandle std_out,
	      const std::string& current_path);

#if IS_DEBUG
	std::vector<Command> ParseCommands(const std::string& line);
#endif

	/// <summary>
	/// Provadi seznam prikazu, dokud nenastane chyba
	/// </summary>
	/// <param name="commands">Seznam prikazu, ktery se ma provest</param>
	void RunCommands(const std::vector<Command>& commands);

	void PreparePipes(std::vector<Command>& commands);

	/// <summary>
	/// Spusti shell - ten bezi, dokud se nezavola exit nebo shutdown
	/// </summary>
	void Run();

	std::pair<bool, std::string> ChangeDirectory(const std::string& path) {
		return { false, "Not Yet Implemented" };
	}

};
