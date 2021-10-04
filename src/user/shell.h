#pragma once

#include <iostream>
#include <ostream>
#include <string>

#include "rtl.h"
#include "../api/api.h"
#include "../../msvc/user/CommandParser.h"

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

	
	auto printNewline() {
		auto whatever = size_t{};
		kiv_os_rtl::Write_File(stdOut, NEWLINE_MESSAGE, strlen(NEWLINE_MESSAGE), whatever);
	}

	auto parseLine(const std::string& line) {
		auto whatever = size_t{};
		auto tokens = commandParser->parseCommands(line);
		auto msgStream = std::stringstream();
		msgStream << "Tokens count: " << tokens.size() << std::endl;
		auto msg = msgStream.str();
		kiv_os_rtl::Write_File(stdOut, msg.data(), msg.size(), whatever);
		for (const auto& token : tokens) {
			kiv_os_rtl::Write_File(stdOut, token.data(), token.size(), whatever);
			printNewline();
		}
	}

};
