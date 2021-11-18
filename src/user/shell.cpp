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

void Shell::Terminate() {
	run = false;
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

auto Shell::PrepareStdIOForSingleCommand(Command& command) const -> std::pair<bool, std::string> {
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


auto Shell::PrepareStdIOForFirstCommand(Command& command, kiv_os::THandle& next_std_in) const -> std::pair<bool, std::string> {

	// Jinak tento prikaz bude mit vystup do dalsiho a tim padem nemuze mit vystup zaroven do prikazu a souboru
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::ToFile) {
		return {false, "Error, command cannot redirect to file and another command at the same time"};
	}

	// File descriptor pro stdin pro tento prikaz
	auto command_std_in = kiv_os::Invalid_Handle;

	// Pokud aktualni prikaz bude cist data ze souboru musime ho otevrit
	if (command.redirect_type == RedirectType::FromFile) {
		// Pokud operace selze, vyhodime chybu, jinak se std_in nastavi spravne
		if (const auto success = kiv_os_rtl::OpenFsFile(command_std_in, command.source_file,
		                                                kiv_os::NOpen_File::fmOpen_Always); !success) {
			return {false, "Error, could not open input file for command: " + command.command_name};
		}
	}
	else {
		// V pripade ze se ctou data z konzole nastavime std_in
		command_std_in = std_in;
	}

	// Nyni musime vytvorit pipu, ktera bude mit stdout pro tento prikaz a stdin pro nasledujici
	auto command_std_out = kiv_os::Invalid_Handle; // std_out pro tento prikaz
	auto next_command_std_in = kiv_os::Invalid_Handle; // std_in pro dalsi prikaz
	// Oba file descriptory museji byt nevalidni, aby je za nas vytvoril OS
	if (const auto success = kiv_os_rtl::CreatePipe(command_std_out, next_command_std_in); !success) {
		CloseCommandFileDescriptors(command); // Zavreme file descriptory, protoze selhalo
		return {false, "Error, could not create output for command: " + command.command_name};
	}

	// Jinak vse probehlo v poradku a nastavime file descriptory
	command.SetPipeFileDescriptors(command_std_in, command_std_out);

	// A na referenci nastavime ziskany std_in pro nasledujici proces
	next_std_in = next_command_std_in;
	return {true, ""};
}



auto Shell::PrepareStdIOForCommands(std::vector<Command>& commands) const -> std::pair<bool, std::string> {
	if (commands.empty()) {
		return {true, ""};
	}

	// Pokud je prikaz pouze jeden, zadne pipy vytvaret nebudeme
	if (commands.size() == 1) {
		return PrepareStdIOForSingleCommand(commands[0]);
	}

	// Tento std_in se bude pouzivat jako std_in pro "dalsi" proces v 1 .. n - 2
	auto command_fd_in = kiv_os::Invalid_Handle;
	// Jinak zacneme vytvaret pro kazdy prikaz pipy
	if (auto [success, err_message] = PrepareStdIOForFirstCommand(commands[0], command_fd_in);
		!success) {
		return { success, err_message };
	}

	// Nyni jedeme od 1 az do n - 1 prikazu (index n - 2) a vytvarime pro ne pipy. Zaroven kontrolujeme nevalidni vstupy
	for (size_t i = 1; i < commands.size() - 1; i += 1) {
		// Ziskame aktualni prikaz, tzn i-ty prvek
		auto& command = commands[i];

		// Prikazy, ktere nejsou na zacatku ani na konci musi mit RedirectType None, jinak bychom presmerovavali
		// do souboru a zaroven do pipy, coz podle api nejde
		if (command.redirect_type != RedirectType::None) {
			CloseCommandListFileDescriptors(commands, i); // Zavreme vsechny file descriptory predchozich prikazu
			kiv_os_rtl::CloseHandle(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
			return {false, "Cannot redirect to file and another command at the same time"};
		}

		// Vstupni file descriptor se vytvoril v minulem prikazu, takze ho staci pouzit a musime vytvorit
		// novou pipe
		auto command_fd_out = kiv_os::Invalid_Handle; // Tento fd chceme vygenerovat
		auto next_command_fd_in = kiv_os::Invalid_Handle;

		// Zkusime vytvorit pipe, pokud nejde vratime chybovou hlasku
		if (const auto success = kiv_os_rtl::CreatePipe(command_fd_out, next_command_fd_in); 
			!success) {
			CloseCommandListFileDescriptors(commands, i); // Zavreme vsechny file descriptory predchozich prikazu
			kiv_os_rtl::CloseHandle(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
			return {false, "Could not create output for command: " + command.command_name};
		}

		// Nastavime prikazu file descriptory
		command.SetPipeFileDescriptors(command_fd_in, command_fd_out);
		command_fd_in = next_command_fd_in;
	}

	// Pro posledni prikaz se pipe vytvorila jiz ve for cyklu a jeho std_in je nastaveny v command_fd_in
	// Takze staci zkontrolovat, jestli prikaz nepresmerovava spatne a nastavime mu bud std_out shellu a nebo soubor
	auto& last_command = commands[commands.size() - 1];
	if (last_command.redirect_type == RedirectType::Both || last_command.redirect_type == RedirectType::FromFile) {
		CloseCommandListFileDescriptors(commands, commands.size() - 1); // Zavreme predchozim souborum file descriptory
		kiv_os_rtl::CloseHandle(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
		return {false, "Error, command cannot read output from another command and file at the same time"};
	}

	// Pokud je vystup do souboru zkusime ho otevrit, jinak zustane std_out
	auto command_fd_out = std_out;
	if (last_command.redirect_type == RedirectType::ToFile) {
		if (const auto success = kiv_os_rtl::OpenFsFile(command_fd_out, last_command.target_file,
			kiv_os::NOpen_File::fmOpen_Always); !success) {
			return { false, "Error, could not open output file for command: " + last_command.command_name };
		}
	}
	last_command.SetPipeFileDescriptors(command_fd_in, command_fd_out); // Nastavime poslednimu souboru file descriptory

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
	if (auto [success, err_message] = PrepareStdIOForCommands(commands); !success) {
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
		// ReSharper disable once CppTooWideScopeInitStatement
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
	for (const auto pid : program_pids) {
		const auto wait_for_list = { pid };
		kiv_os_rtl::WaitFor(wait_for_list);
		auto exit_code = kiv_os::NOS_Error::Success;
		kiv_os_rtl::ReadExitCode(pid, exit_code); // Precteme exit code pro odstraneni z tabulky
	}
}
