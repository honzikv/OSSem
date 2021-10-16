
#include "Tests/UnitTests.h"

#include <array>
#include <iostream>
#include <memory>

#include "shell.h"
#include "rtl.h"
#include "Shell/ShellInterpreter.h"

// Debug pro run testu
#define IS_DEBUG true


size_t __stdcall shell(const kiv_hal::TRegisters& regs) {

#if IS_DEBUG
	TestRunner::runTests();
#endif

	// stdin a stdout jsou asi schvalne predany v registrech ?
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	// Pole jako objekt - k pointeru se pristupuje pres buffer.data()
	auto buffer = std::array<char, BUFFER_SIZE>(); // buffer pro nacitani user inputu
	size_t counter;

	// constexpr auto intro = "Vitejte v kostre semestralni prace z KIV/OS.\n"
	// 	"Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
	// kiv_os_rtl::Write_File(std_out, intro, strlen(intro), counter);


	// Shell interpreter parsuje prikazy a vola prislusne sluzby OS
	// Unique pointer (make_unique) vytvori objekt na heapu, takze se sam smaze pri ukonceni scopu
	const auto shellInterpreter = std::make_unique<ShellInterpreter>(regs, std_in, std_out);

	constexpr auto prompt = "C:\\>"; // todo: filesystem integration
	do {
		kiv_os_rtl::Write_File(std_out, prompt, strlen(prompt), counter);

		if (kiv_os_rtl::Read_File(std_in, buffer.data(), BUFFER_SIZE, counter)) {
			if (counter == BUFFER_SIZE) {
				counter--;
			}

			buffer[counter] = 0; //udelame z precteneho vstup null-terminated retezec

			// Vypis newline do konzole
			// kiv_os_rtl::Write_File(std_out, NEWLINE_SYMBOL, strlen(NEWLINE_SYMBOL), counter);
			// Parsovani vstupu z konzole
			// shellInterpreter->parseLine(buffer.data());

			// Puvodni - pouze echo do konzole
			// kiv_os_rtl::Write_File(std_out, buffer.data(), strlen(buffer.data()), counter); //a vypiseme ho

			// Vypis newline do konzole
			// kiv_os_rtl::Write_File(std_out, NEWLINE_SYMBOL, strlen(NEWLINE_SYMBOL), counter);

		}
		else {
			break; //EOF
		}
	}
	while (strcmp(buffer.data(), EXIT_COMMAND) != 0); // dokud nezada user EXIT

	return 0;

	// return testShellParsing1();
}

std::vector<Command> ShellInterpreter::parseCommands(const std::string& line) {
	return commandParser->parseCommands(line);
}

auto ShellInterpreter::executeCommand(const Command& command) -> void {
	if (command.commandName == "toggledebug") {
		toggleDebug();
	}

	// TODO impl
}

auto ShellInterpreter::toggleDebug() -> void {
	debugOn = !debugOn;
}
