#pragma once
#include <memory>
#include <unordered_set>
#include <array>
#include "../api/api.h"
#include "CommandParser.h"

constexpr auto NEWLINE_MESSAGE = "\n";
constexpr size_t BUFFER_SIZE = 256;
constexpr auto NEWLINE_SYMBOL = "\n";
constexpr auto EXIT_COMMAND = "exit";

/**
 * Trida pro zpracovani prikazu z shellu
 *
 * Wrapper pro shell() funkci, aby v byl kod aspon trochu citelny
 */
class ShellInterpreter {

	/**
	 * Vsechny dostupne programy v OS
	 */
	std::unordered_set<std::string> possiblePrograms = {
		"type", "md", "rd", "dir", "echo", "find", "sort", "rgen", "tasklist", "freq", "shutdown", "cd",

		// Custom
		"toggledebug"
	};

	// reference na registry
	const kiv_hal::TRegisters& registers;

	// reference na stdin
	const kiv_os::THandle& stdIn;

	// reference na stdout
	const kiv_os::THandle& stdOut;

	// interni objekt na parsovani dat - alokace na heapu
	const std::unique_ptr<CommandParser> commandParser = std::make_unique<CommandParser>();

	// debug mode
	bool debugOn = false;

	// Pocitadlo pro tisk do konzole
	size_t counter;

	// IO buffer
	std::array<char, BUFFER_SIZE> buffer;

public:
	// Ctor ziska z funkce shell vsechny registry + handle na stdin a stdout
	ShellInterpreter(const kiv_hal::TRegisters& registers, const kiv_os::THandle& stdIn,
	                 const kiv_os::THandle& stdOut) :
		registers(registers),
		stdIn(stdIn),
		stdOut(stdOut) {}

	std::vector<Command> parseCommands(const std::string& line);

	auto parseLine(const std::string& line);

	auto executeCommand(const Command& command) -> void;

	auto toggleDebug() -> void;

};
