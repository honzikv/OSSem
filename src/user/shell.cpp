#include "Tests/UnitTests.h"

#include <array>
#include <iostream>
#include <memory>

#include "shell_commands.h"
#include "Shell/Shell.h"
#include "Utils/Logging.h"


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

Shell::Shell(const kiv_hal::TRegisters& registers, kiv_os::THandle std_in, kiv_os::THandle std_out,
             const std::string& current_path):
	registers(registers),
	std_in(std_in),
	std_out(std_out),
	current_path(current_path) {}

std::vector<Command> Shell::ParseCommands(const std::string& line) {
	return command_parser.ParseCommands(line);
}

void Shell::RunCommands(const std::vector<Command>& commands) {
	for (auto i = 0; i < commands.size(); i += 1) {
		const auto& command = commands[i];
		const auto params = command.Params();

		// Shell neni mozne volat pres syscall, takze se pro nej musi vytvorit v shellu specialni funkce
		if (command.command_name == "cd") {
			const auto [success, errorMessage] = ChangeDirectory(params);
			if (!success) {
				// Pokud dosl
				WriteLine(errorMessage);
				break;
			}
			continue;
		}

		if (command.command_name == "exit") {
			Terminate();
		}
	}
}

std::pair<bool, std::string> Shell::PreparePipes(std::vector<Command>& commands) {
	for (size_t i = 0; i < commands.size() - 1; i += 1) {
		size_t j = i + 1;

		auto& command = commands[i];
		// Pokud ma prikaz redirect type pro vstup i vystup a zaroven chceme predat data procesu vyhodime chybu
		// (nelze zkonstruovat kvuli api)
		if (command.redirect_type == RedirectType::Both) {
			return { false, "Error, cannot redirect to a program and file at the same time" };
		}

		// Pokud mame | program < file.txt vyhodime chybu
		if (command.redirect_type == RedirectType::FromFile && i != 0) {
			return { false, "Error, cannot read input from file and program at the same time" };
		}

		// Pokud mame program > out.txt | another_program vyhodime chybu
		if (command.redirect_type == RedirectType::ToFile && j != commands.size() - 1) {
			return { false, "Error, cannot redirect input to file and program at the same time" };
		}


	}
}

void Shell::Run() {
	while (run) {
		Write(current_path); // Zapiseme aktualni cestu

		// Vyresetujeme buffer
		std::fill_n(buffer.data(), buffer.size(), 0);

		// Precteme uzivatelsky vstup
		size_t bytesRead;
		const auto read_success = kiv_os_rtl::Read_File(std_in, buffer.data(), buffer.size(), bytesRead);
		if (!read_success) {
			// Pokud EOT ukoncime while loop
			break;
		}

		// Ziskame uzivatelsky vstup, ktery prevedeme na std::sting (vyresi za nas \0 terminaci)
		auto user_input = std::string(buffer.begin(),
		                              bytesRead >= buffer.size()
			                              ? buffer.end()
			                              : buffer.begin() + bytesRead);

		// Vytvorime seznam prikazu a zkusime je rozparsovat z uzivatelskeho vstupu
		auto commands = std::vector<Command>();
		try {
			commands = command_parser.ParseCommands(user_input);
			WriteLine("");
		}
		catch (ParseException& ex) {
			// Pri chybe vypiseme hlasku do konzole a restartujeme while loop
			WriteLine(ex.what());
			continue;
		}

		RunCommands(commands); // Provedeme vsechny prikazy
	}
}
