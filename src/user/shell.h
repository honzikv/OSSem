#pragma once

#include <iostream>
#include <ostream>
#include <string>

#include "rtl.h"
#include "..\api\api.h"
#include "../../msvc/user/CommandParser.h"

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters& regs);

const std::string EXIT_COMMAND = "exit"; // NOLINT(clang-diagnostic-exit-time-destructors)


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

class ShellInterpreter
{
	const kiv_hal::TRegisters& registers;
	const kiv_os::THandle& stdIn, stdOut;

public:
	ShellInterpreter(const kiv_hal::TRegisters& registers, const kiv_os::THandle& stdIn, const kiv_os::THandle& stdOut):
		registers(registers), stdIn(stdIn), stdOut(stdOut)
	{
	}

	inline void parseLine(const std::string& line, size_t& counter)
	{
		auto commandParser = CommandParser();
		auto tokens = commandParser.parseCommand(line);

		// kiv_os_rtl::Write_File(stdOut, line.c_str(), line.size(), counter);
		kiv_os_rtl::Write_File(stdOut, tokens[0].c_str(), line.size(), counter);
	}
};
