#include "shell.h"

#include <array>
#include <iostream>
#include <memory>

// todo remove later
auto testShellParsing1() {
	auto commandParser = CommandParser();
	std::string line = "ls -a > out.txt|ls -a lmfao --aghagagagahah xDDD > out.you.go|cat .";

	std::cout << line << std::endl;

	auto commands = commandParser.parseCommands(line);

	for (const auto& command : commands) {
		std::cout << "[" << std::endl;
		std::cout << "\tcommand: " << command.commandName << std::endl;

		if (command.redirectType != RedirectType::None) {
			if (command.redirectType == RedirectType::FromFile) {
				std::cout << "\tredirect type: from file" << std::endl;
			}
			else {
				std::cout << "\tredirect type: to file" << std::endl;
			}
			std::cout << "\tfile name: " << command.file << std::endl;
		}
		else {
			std::cout << "\tredirect type: none" << std::endl;
		}

		std::cout << "\tparams: ";
		for (const auto& param : command.params) {
			std::cout << param;
		}
		std::cout << std::endl;
		std::cout << "]" << std::endl << std::endl;

	}
	return 0;
}

size_t __stdcall shell(const kiv_hal::TRegisters& regs) {
	// // stdin a stdout jsou asi schvalne predany v registrech ?
	// const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	// const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	//
	// // Pole jako objekt - k pointeru se pristupuje pres buffer.data()
	// auto buffer = std::array<char, BUFFER_SIZE>(); // buffer pro nacitani user inputu
	// size_t counter;
	//
	// // constexpr auto intro = "Vitejte v kostre semestralni prace z KIV/OS.\n"
	// // 	"Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
	// // kiv_os_rtl::Write_File(std_out, intro, strlen(intro), counter);
	//
	//
	// // Shell interpreter parsuje prikazy a vola prislusne sluzby OS
	// // Unique pointer (make_unique) vytvori objekt na heapu, takze se sam smaze pri ukonceni scopu
	// const auto shellInterpreter = std::make_unique<ShellInterpreter>(regs, std_in, std_out);
	//
	// constexpr auto prompt = "C:\\>"; // todo: filesystem integration
	// do {
	// 	kiv_os_rtl::Write_File(std_out, prompt, strlen(prompt), counter);
	//
	// 	if (kiv_os_rtl::Read_File(std_in, buffer.data(), BUFFER_SIZE, counter)) {
	// 		if (counter == BUFFER_SIZE) {
	// 			counter--;
	// 		}
	//
	// 		buffer[counter] = 0; //udelame z precteneho vstup null-terminated retezec
	//
	// 		// Vypis newline do konzole
	// 		// kiv_os_rtl::Write_File(std_out, NEWLINE_SYMBOL, strlen(NEWLINE_SYMBOL), counter);
	// 		// Parsovani vstupu z konzole
	// 		// shellInterpreter->parseLine(buffer.data());
	// 		std::cout << std::endl;
	// 		
	// 		// Puvodni - pouze echo do konzole
	// 		// kiv_os_rtl::Write_File(std_out, buffer.data(), strlen(buffer.data()), counter); //a vypiseme ho
	//
	// 		// Vypis newline do konzole
	// 		// kiv_os_rtl::Write_File(std_out, NEWLINE_SYMBOL, strlen(NEWLINE_SYMBOL), counter);
	//
	// 	}
	// 	else {
	// 		break; //EOF
	// 	}
	// }
	// while (strcmp(buffer.data(), EXIT_COMMAND) != 0); // dokud nezada user EXIT
	//
	// return 0;

	return testShellParsing1();
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

auto ShellInterpreter::parseLine(const std::string& line) {
	const auto commands = commandParser->parseCommands(line);

	for (const auto& command : commands) {
		executeCommand(command);
		std::cout << command.commandName << std::endl;
	}
}
