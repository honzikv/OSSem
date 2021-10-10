#pragma once

#include <string>

#include "rtl.h"
#include "../api/api.h"
#include "../../msvc/user/CommandParser.h"
#include <unordered_set>

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters& regs);

constexpr auto NEWLINE_MESSAGE = "\n";

//nasledujici funkce si dejte do vlastnich souboru
//cd nemuze byt externi program, ale vestavny prikaz shellu!
extern "C" size_t __stdcall type(const kiv_hal::TRegisters& regs) { return 0; };
extern "C" size_t __stdcall md(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall rd(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall dir(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall echo(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall find(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall sort(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall freq(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters& regs) { return 0; }
extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters& regs) { return 0; }

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
		"type", "md", "rd", "dir", "echo", "find", "sort", "rgen", "tasklist", "freq", "shutdown", "cd"
	};

	// reference na registry
	const kiv_hal::TRegisters& registers;

	// reference na stdin
	const kiv_os::THandle& stdIn;

	// reference na stdout
	const kiv_os::THandle& stdOut;

	// interni objekt na parsovani dat - alokace na heapu
	const std::unique_ptr<CommandParser> commandParser = std::make_unique<CommandParser>();

public:
	// Ctor ziska z funkce shell vsechny registry + handle na stdin a stdout
	ShellInterpreter(const kiv_hal::TRegisters& registers, const kiv_os::THandle& stdIn, const kiv_os::THandle& stdOut):
		registers(registers),
		stdIn(stdIn),
		stdOut(stdOut) { }


	auto parseLine(const std::string& line) {
		const auto commands = commandParser->parseCommands(line);

		for (const auto& command : commands) {
			executeCommand(command);
		}
	}

	auto executeCommand(const Command& command) -> void {
		// TODO impl
	}

};
