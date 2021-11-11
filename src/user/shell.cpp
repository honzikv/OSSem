#include "Tests/UnitTests.h"

#include <array>
#include <memory>

#include "shell_commands.h"
#include "Shell/Shell.h"
#include "Utils/Logging.h"


size_t __stdcall shell(const kiv_hal::TRegisters& regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto shell = std::make_unique<Shell>(regs, std_in, std_out, "C:\\>");

	// Spustime shell
	shell->Run();

	return static_cast<size_t>(kiv_os::NOS_Error::Success);

}

void Shell::Write(const std::string& message) const {
	size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
	kiv_os_rtl::WriteFile(std_out, message.data(), message.size(), written);
}

void Shell::WriteLine(const std::string& message) const {
	Write(message + NEWLINE_SYMBOL);
}

Shell::Shell(const kiv_hal::TRegisters& registers, const kiv_os::THandle std_in, const kiv_os::THandle std_out,
             std::string current_path):
	registers(registers),
	std_in(std_in),
	std_out(std_out),
	current_working_dir(std::move(current_path)) {}

std::vector<Command> Shell::ParseCommands(const std::string& line) const {
	return command_parser.ParseCommands(line);
}

auto Shell::PreparePipeForSingleCommand(Command& command) const -> std::pair<bool, std::string> {
	// File descriptory pro vstup a vystup
	auto command_fd_in = kiv_os::Invalid_Handle;
	auto command_fd_out = kiv_os::Invalid_Handle;

	// Pokud neni presmerovani z ani do souboru, nastavime pro prikaz stdin a stdout
	if (command.redirect_type == RedirectType::None) {
		command.SetPipeFileDescriptors(std_in, std_out);
		return {true, ""};
	}

	// Zkusime nastavit vstupni soubor
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::FromFile) {
		if (const auto success = kiv_os_rtl::OpenFsFile(command_fd_in, command.source_file,
		                                                kiv_os::NOpen_File::fmOpen_Always); !success) {
			return {false, "Error, could not open input file for command: " + command.command_name};
		}
	}

	// Zkusime nastavit vystupni soubor
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::ToFile) {
		if (const auto success = kiv_os_rtl::OpenFsFile(command_fd_out, command.target_file,
		                                                kiv_os::NOpen_File::fmOpen_Always); !success) {
			CloseCommandFileDescriptors(command);
			return {false, "Error, could not open output file for command: " + command.command_name};
		}
	}

	// Pokud jsme se dostali az sem, vse dobehlo v poradku
	return {true, ""};
}


auto Shell::PreparePipeForFirstCommand(Command& command,
                                       const bool is_next_command) const -> std::pair<bool, std::string> {

	// Pokud neni dalsi prikaz pripravime file descriptory pouze pro jeden
	if (!is_next_command) {
		return PreparePipeForSingleCommand(command);
	}

	// Jinak tento prikaz bude mit vystup do dalsiho a tim padem nemuze mit vystup zaroven do prikazu a souboru
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::ToFile) {
		return {false, "Error, command cannot redirect to file and another command at the same time"};
	}

	// File descriptory pro vstup a vystup
	auto command_fd_in = kiv_os::Invalid_Handle;
	auto command_fd_out = kiv_os::Invalid_Handle;

	if (command.redirect_type == RedirectType::FromFile) {
		// Otevreme soubor a zkusime nastavit file descriptor, pokud nelze vyhodime chybu
		if (const auto success = kiv_os_rtl::OpenFsFile(command_fd_in, command.source_file,
		                                                kiv_os::NOpen_File::fmOpen_Always); !success) {
			return {false, "Error, could not open input file for command: " + command.command_name};
		}
	}
	else {
		// Jinak nastavime jako vstup stdin
		command_fd_in = std_in;
	}

	// Pro vystupni file descriptor musime vytvorit pipe
	if (const auto success = kiv_os_rtl::CreatePipe(command_fd_in, command_fd_out); !success) {
		CloseCommandFileDescriptors(command); // Zavreme file descriptory, protoze selhalo
		return {false, "Error, could not create output for command: " + command.command_name};
	}

	// Jinak vse probehlo v poradku a nastavime file descriptory
	command.SetPipeFileDescriptors(command_fd_in, command_fd_out);
	return {true, ""};
}


std::pair<bool, std::string> Shell::PreparePipeForLastCommand(const Command& second_last, Command& last) {
	if (last.redirect_type == RedirectType::Both || last.redirect_type == RedirectType::FromFile) {
		return {false, "Error, command cannot read output from another command and file at the same time"};
	}

	const auto command_fd_in = second_last.GetOutputFileDescriptor();
	auto command_fd_out = kiv_os::Invalid_Handle;

	if (last.redirect_type == RedirectType::ToFile) {
		if (const auto success = kiv_os_rtl::OpenFsFile(command_fd_out, last.target_file,
		                                                kiv_os::NOpen_File::fmOpen_Always); !success) {
			return {false, "Error, could not open output file for command: " + last.command_name};
		}
	}

	last.SetPipeFileDescriptors(command_fd_in, command_fd_out);
	return {true, ""};
}


auto Shell::PreparePipes(std::vector<Command>& commands) const -> std::pair<bool, std::string> {
	if (commands.empty()) {
		return {true, ""};
	}

	if (commands.size() == 1) {
		return PreparePipeForFirstCommand(commands[0], false);
	}

	// Zkusime vytvorit pipe pro prvni soubor, pokud selze vratime chybovou hlasku
	if (auto [first_command_result, err_message] = PreparePipeForFirstCommand(commands[0], true);
		!first_command_result) {
		return {first_command_result, err_message};
	}

	for (size_t i = 1; i < commands.size() - 2; i += 1) {
		auto& previous_command = commands[i - 1]; // reference na predchozi prikaz, ze ktereho potrebujeme vystupni fd
		// Ziskame aktualni prikaz, tzn i-ty prvek
		auto& command = commands[i];

		// Prikazy, ktere nejsou na zacatku ani na konci musi mit RedirectType None, jinak bychom presmerovavali
		// do souboru a zaroven do pipy, coz podle api nejde
		if (command.redirect_type != RedirectType::None) {
			CloseCommandListFileDescriptors(commands, i); // Zavreme vsechny file descriptory predchozich prikazu
			return {false, "Cannot redirect to file and another command at the same time"};
		}

		// File descriptor pro vstup ziskame z predchoziho prikazu jako jeho vystup
		auto command_fd_in = previous_command.GetInputFileDescriptor();
		auto command_fd_out = kiv_os::Invalid_Handle; // Tento fd chceme vygenerovat

		// Zkusime vytvorit pipe, pokud nejde vratime chybovou hlasku
		if (const auto success = kiv_os_rtl::CreatePipe(command_fd_in, command_fd_out); !success) {
			CloseCommandListFileDescriptors(commands, i); // Zavreme vsechny file descriptory predchozich prikazu
			return {false, "Could not create output for command: " + command.command_name};
		}

		command.SetPipeFileDescriptors(command_fd_in, command_fd_out);
	}

	// Zkusime vytvorit pipe pro posledni soubor, pokud selze vyhodime chybovou hlasku
	const auto& second_last_command = commands[commands.size() - 2];
	auto& last_command = commands[commands.size() - 1];
	if (auto [last_command_result, err_message] = PreparePipeForLastCommand(second_last_command, last_command);
		!last_command_result) {
		// Pro posledni prikaz nastala chyba a nenastavily se file descriptory, takze zavreme vsechny ostatni
		CloseCommandListFileDescriptors(commands, commands.size() - 1);
		return {last_command_result, err_message};
	}

	return {true, ""};
}

void Shell::CloseCommandFileDescriptors(const Command& command) const {
	// File descriptor lze zavrit pokud je otevreny a neni to standardni vstup / vystup shellu
	if (command.InputFileDescriptorOpen() && command.GetInputFileDescriptor() != std_in) {
		kiv_os_rtl::CloseHandle(command.GetInputFileDescriptor());
	}
	
	if (command.OutputFileDescriptorOpen() && command.GetOutputFileDescriptor() != std_out) {
		kiv_os_rtl::CloseHandle(command.GetOutputFileDescriptor());
	}
}

void Shell::CloseCommandListFileDescriptors(const std::vector<Command>& commands, const size_t count) const {
	for (size_t i = 0; i < count; i += 1) {
		CloseCommandFileDescriptors(commands[i]);
	}
}

void Shell::CloseCommandListFileDescriptors(const std::vector<Command>& commands, size_t idx_start,
                                            size_t idx_end) const {
	for (size_t i = idx_start; i <= idx_end; i += 1) {
		CloseCommandFileDescriptors(commands[i]);
	}
}


void Shell::Run() {
	while (run) {
		Write(current_working_dir); // Zapiseme aktualni cestu

		// Vyresetujeme buffer
		std::fill_n(buffer.data(), buffer.size(), 0);

		// Precteme uzivatelsky vstup
		size_t bytesRead;
		if (const auto read_success = kiv_os_rtl::ReadFile(std_in, buffer.data(), buffer.size(), bytesRead);
			!read_success) {
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

std::pair<bool, std::string> Shell::ChangeDirectory(const Command& command) {

	// Zavolame sluzbu pro zmenu adresare
	const auto params = command.GetRtlParams();
	if (const auto success = kiv_os_rtl::SetWorkingDir(params); !success) {
		return {false, "Error, could not change directory for cd with arguments: " + params};
	}
	
	// Pokud zmena adresare probehla v poradku musime jeste aktualizovat stav v konzoli
	constexpr auto new_dir_buffer_size = BUFFER_SIZE / 2; // velikost bufferu pro zapsani vysledku
	auto new_directory_buffer = std::array<char, new_dir_buffer_size>(); // buffer pro zapsani vysledku
	auto new_directory_str_size = uint32_t{0};
	if (const auto success = kiv_os_rtl::GetWorkingDir(new_directory_buffer.data(), new_dir_buffer_size,
	                                                   new_directory_str_size); !success) {
		Terminate(); // Tento stav asi nikdy nenastane, ale pro jistotu
		return {false, "Critical error ocurred, cannot get current working directory. Shell will close."};
	}
	
	current_working_dir = std::string(new_directory_buffer.begin(), new_directory_buffer.begin() + new_dir_buffer_size);
	return {true, ""};
}


void Shell::RunCommands(std::vector<Command>& commands) {

	// Pokud doslo k chybe vypiseme ji a prikazy nespustime
	if (auto [success, err_message] = PreparePipes(commands); !success) {
		WriteLine(err_message);
		return;
	}

	// Jinak zacneme vytvaret procesy pro kazdy prikaz
	auto program_pids = std::vector<kiv_os::THandle>();
	for (auto i = 0; i < commands.size(); i += 1) {
		const auto& command = commands[i];

		// Shell neni mozne volat pres syscall, takze se pro nej musi vytvorit v shellu specialni funkce
		if (command.command_name == "cd") {
			if (const auto [success, errorMessage] = ChangeDirectory(command); !success) {
				WriteLine(errorMessage);
				break;
			}
			continue;
		}

		// Stejne tak exit neni proces, ale ukonceni konzole
		if (command.command_name == "exit") {
			// TODO ukoncit vzdy nebo jenom kdyz se explicitne zavola exit?
			Terminate();
			return;
		}

		// Jinak vytvorime novy proces
		auto pid = kiv_os::Invalid_Handle; // pid pro cekani
		const auto success = kiv_os_rtl::CreateProcess(command.command_name, command.GetRtlParams(),
			command.GetInputFileDescriptor(),
			command.GetOutputFileDescriptor(), pid);

		if (!success) {
			switch (kiv_os_rtl::GetLastError()) {
			case kiv_os::NOS_Error::Out_Of_Memory: {
				WriteLine("Error, OS does not have enough memory for next process.");
				break;
			}

			case kiv_os::NOS_Error::Invalid_Argument: {
				WriteLine("Error specified program: " + command.command_name + " does not exist!");
				break;
			}

			default: {
				WriteLine("An unknown OS error has occurred.");
				break;
			}
			}

			// Protoze nastala chyba, nebudeme vykonavat dalsi procesy a zavreme jejich file descriptory
			CloseCommandListFileDescriptors(commands, i, commands.size() - 1);
			return;
		}

		program_pids.push_back(pid);
	}

	// Shell pocka na dokonceni vsech procesu
	for (const auto& pid : program_pids) {
		kiv_os_rtl::WaitFor({ pid });
		auto exit_code = kiv_os::NOS_Error::Success;
		kiv_os_rtl::ReadExitCode(pid, exit_code); // Precteme exit code pro odstraneni z tabulky
	}
}