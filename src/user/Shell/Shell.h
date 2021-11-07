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
 * 
 * Wrapper pro shell() funkci pro lepsi citelnost kodu
 */
class Shell {
	
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

	// IO buffer
	std::array<char, BUFFER_SIZE> buffer;

public:
	// Ctor ziska z funkce shell vsechny registry + handle na stdin a stdout
	Shell(const kiv_hal::TRegisters& registers, const kiv_os::THandle& stdIn,
	                 const kiv_os::THandle& stdOut) :
		registers(registers),
		stdIn(stdIn),
		stdOut(stdOut) {}

	std::vector<Command> parseCommands(const std::string& line);
	

	void executeCommand(const Command& command);

	void toggleDebug();

};
