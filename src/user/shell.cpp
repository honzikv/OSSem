#include "Tests/UnitTests.h"

#include <array>
#include <iostream>
#include <memory>

#include "shell_commands.h"
#include "Shell/Shell.h"



size_t __stdcall shell(const kiv_hal::TRegisters& regs) {

#if IS_DEBUG
	TestRunner::runTests();
#endif
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	
	const auto shell = std::make_unique<Shell>(regs, std_in, std_out, "C:\\");

	// Spustime shell
	shell->Run();

	return static_cast<size_t>(kiv_os::NOS_Error::Success);
	
}

void Shell::Write(const std::string& message) const {
	size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
	kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
}

void Shell::WriteLine(const std::string& message) const {
	Write(message + NEWLINE_SYMBOL);
}

Shell::Shell(const kiv_hal::TRegisters& registers, const kiv_os::THandle std_in, const kiv_os::THandle std_out,
             const std::string& current_path):
	registers(registers),
	std_in(std_in),
	std_out(std_out),
	current_path(current_path) {}

std::vector<Command> Shell::ParseCommands(const std::string& line) {
	return command_parser->ParseCommands(line);
}
