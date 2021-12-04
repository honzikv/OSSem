#include "Tests/UnitTests.h"

#include <array>
#include <memory>

#include "shell_commands.h"
#include "Shell/Shell.h"
#include "Utils/Logging.h"
#include "Utils/StringUtils.h"

extern bool echo_on;

size_t __stdcall shell(const kiv_hal::TRegisters& regs) {
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	constexpr auto working_dir_str_size = 64;
	auto working_dir = std::array<char, working_dir_str_size>();
	uint32_t str_size = 0;

	// Aktualni pracovni adresar
	const auto current_path = kiv_os_rtl::Get_Working_Dir(working_dir.data(), working_dir_str_size, str_size);

	const auto path = std::string(working_dir.data(), str_size);
	const auto shell = std::make_unique<Shell>(regs, std_in, std_out, path);

	// Spustime shell
	shell->Run();

	return static_cast<size_t>(kiv_os::NOS_Error::Success);

}

void Shell::Write(const std::string& message) const {
	size_t written; // Pro toto predpokladame, ze se zapisi byty vsechny
	kiv_os_rtl::Write_File(std_out, message.data(), message.size(), written);
}

void Shell::Write_Line(const std::string& message) const {
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

std::vector<Command> Shell::Parse_Commands(const std::string& line) const {
	return command_parser.Parse_Commands(line);
}

auto Shell::Prepare_Stdio_For_Single_Command(Command& command) const -> std::pair<bool, std::string> {
	// File descriptory pro vstup a vystup
	auto command_std_in = kiv_os::Invalid_Handle;
	auto command_std_out = kiv_os::Invalid_Handle;

	// Pokud neni presmerovani z ani do souboru, nastavime pro prikaz stdin a stdout
	if (command.redirect_type == RedirectType::None) {
		command.Set_Stdio_File_Descriptors(std_in, std_out);
		return {true, ""};
	}

	// Zkusime nastavit vstupni file descriptor
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::FromFile) {
		if (const auto success = kiv_os_rtl::Open_File(command_std_in, command.source_file,
		                                               kiv_os::NOpen_File::fmOpen_Always,
		                                               kiv_os::NFile_Attributes::Read_Only);
			!success) {
			return {false, "Error, could not open input file for command: " + command.command_name};
		}
	}
	else {
		// V pripade ze se ctou data z konzole nastavime std_in
		command_std_in = std_in;
	}

	// Uspech - nastavime file descriptor pro cteni
	command.Set_Std_In_File_Descriptor(command_std_in);

	// Zkusime nastavit vystupni soubor
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::ToFile) {
		if (const auto success = kiv_os_rtl::Open_File(command_std_out, command.target_file,
		                                               static_cast<kiv_os::NOpen_File>(0),
		                                               kiv_os::NFile_Attributes::Archive); !success) {
			Close_Command_Std_In(command);
			return {false, "Error, could not open output file for command: " + command.command_name};
		}
	}
	else {
		command_std_out = std_out;
	}

	// Uspech - nastavime file descriptor
	command.Set_Std_Out_File_Descriptor(command_std_out);

	// Pokud jsme se dostali az sem, vse dobehlo v poradku
	return {true, ""};
}


auto Shell::Prepare_Stdio_For_First_Command(Command& command,
                                            kiv_os::THandle& next_std_in) const -> std::pair<bool, std::string> {

	// Jinak tento prikaz bude mit vystup do dalsiho a tim padem nemuze mit vystup zaroven do prikazu a souboru
	if (command.redirect_type == RedirectType::Both || command.redirect_type == RedirectType::ToFile) {
		return {false, "Error, command cannot redirect to file and another command at the same time"};
	}

	// File descriptor pro stdin pro tento prikaz
	auto command_std_in = kiv_os::Invalid_Handle;

	// Pokud aktualni prikaz bude cist data ze souboru musime ho otevrit
	if (command.redirect_type == RedirectType::FromFile) {
		// Pokud operace selze, vyhodime chybu, jinak se std_in nastavi spravne
		if (const auto success = kiv_os_rtl::Open_File(command_std_in, command.source_file,
		                                               kiv_os::NOpen_File::fmOpen_Always,
		                                               kiv_os::NFile_Attributes::Read_Only); !success) {
			return {false, "Error, could not open input file for command: " + command.command_name};
		}
	}
	else {
		// V pripade ze se ctou data z konzole nastavime std_in
		command_std_in = std_in;
	}

	command.Set_Std_In_File_Descriptor(command_std_in);

	// Nyni musime vytvorit pipu, ktera bude mit stdout pro tento prikaz a stdin pro nasledujici
	auto command_std_out = kiv_os::Invalid_Handle; // std_out pro tento prikaz
	auto next_command_std_in = kiv_os::Invalid_Handle; // std_in pro dalsi prikaz

	// Oba file descriptory museji byt nevalidni, aby je za nas vytvoril OS
	if (const auto success = kiv_os_rtl::Create_Pipe(command_std_out, next_command_std_in); !success) {
		// Doslo k chybe, takze musime zavrit stdin programu
		Close_Command_Std_In(command);
		return {false, "Error, could not create output file for command: " + command.command_name};
	}

	// Jinak vse probehlo v poradku a nastavime file descriptory
	command.Set_Stdio_File_Descriptors(command_std_in, command_std_out);

	// A na referenci nastavime ziskany std_in pro nasledujici proces
	next_std_in = next_command_std_in;
	return {true, ""};
}


auto Shell::Prepare_Stdio_For_Commands(std::vector<Command>& commands) const -> std::pair<bool, std::string> {
	if (commands.empty()) {
		return {true, ""};
	}

	// Pokud je prikaz pouze jeden, zadne pipy vytvaret nebudeme
	if (commands.size() == 1) {
		return Prepare_Stdio_For_Single_Command(commands[0]);
	}

	// Tento std_in se bude pouzivat jako std_in pro "dalsi" proces v 1 .. n - 2
	auto command_fd_in = kiv_os::Invalid_Handle;

	// Zkusime pripravit stdio pro prvni prikaz
	if (auto [success, err_message] = Prepare_Stdio_For_First_Command(commands[0], command_fd_in);
		!success) {
		return {success, err_message};
	}

	// Nyni jedeme od 1 az do n - 1 prikazu (index n - 2) a vytvarime pro ne pipy
	for (size_t i = 1; i < commands.size() - 1; i += 1) {
		// Ziskame aktualni prikaz, tzn i-ty prvek
		auto& command = commands[i];

		// Prikazy, ktere nejsou na zacatku ani na konci musi mit RedirectType None protoze nemuzeme mit stdin a stdout do
		// vice souboru najednou
		if (command.redirect_type != RedirectType::None) {
			// Nesmyslny vstup od uzivatele. Musime zavrit stdio pro vsechny predchozi prikazy
			// i - index a pocet prikazu, ktere jsme uz zpracovali a je pro ne otevrene stdio
			Close_Command_List_Stdio(commands, i);
			kiv_os_rtl::Close_File_Descriptor(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
			return {false, "Cannot redirect to file and another command at the same time"};
		}

		// Vstupni file descriptor se vytvoril v minulem prikazu, takze ho staci pouzit a musime vytvorit
		// novou pipe
		auto command_fd_out = kiv_os::Invalid_Handle; // Tento fd chceme vygenerovat
		auto next_command_fd_in = kiv_os::Invalid_Handle;

		// Zkusime vytvorit pipe, pokud nejde vratime chybovou hlasku
		if (const auto success = kiv_os_rtl::Create_Pipe(command_fd_out, next_command_fd_in);
			!success) {
			Close_Command_List_Stdio(commands, i); // Zavreme vsechny file descriptory predchozich prikazu
			kiv_os_rtl::Close_File_Descriptor(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
			return {false, "Could not create output file for command: " + command.command_name};
		}

		// Nastavime prikazu file descriptory
		command.Set_Stdio_File_Descriptors(command_fd_in, command_fd_out);
		command_fd_in = next_command_fd_in;
	}

	// Pro posledni prikaz se pipe vytvorila jiz ve for cyklu a jeho std_in je nastaveny v command_fd_in
	// Takze staci zkontrolovat, jestli prikaz nepresmerovava spatne a nastavime mu bud std_out shellu a nebo soubor
	auto& last_command = commands[commands.size() - 1];
	if (last_command.redirect_type == RedirectType::Both || last_command.redirect_type == RedirectType::FromFile) {
		Close_Command_List_Stdio(commands, commands.size() - 1);
		// Zavreme predchozim souborum file descriptory
		kiv_os_rtl::Close_File_Descriptor(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
		return {false, "Error, command cannot read output from another command and file at the same time"};
	}

	// Pokud je vystup do souboru zkusime ho otevrit, jinak zustane std_out
	auto command_fd_out = std_out;
	if (last_command.redirect_type == RedirectType::ToFile) {
		if (const auto success = kiv_os_rtl::Open_File(command_fd_out, last_command.target_file,
		                                               static_cast<kiv_os::NOpen_File>(0),
		                                               kiv_os::NFile_Attributes::Archive); !success) {
			Close_Command_List_Stdio(commands, commands.size() - 1);
			// Zavreme predchozim souborum file descriptory
			kiv_os_rtl::Close_File_Descriptor(command_fd_in); // zavreme std_in, ktery mel byt pro tento proces
			return {false, "Error, could not open output file for command: " + last_command.command_name};
		}
	}
	last_command.Set_Stdio_File_Descriptors(command_fd_in, command_fd_out);
	// Nastavime poslednimu souboru file descriptory

	return {true, ""};
}


void Shell::Close_Command_Std_In(const Command& command) const {
	if (command.Is_Input_File_Descriptor_Open() && command.Get_Input_File_Descriptor() != std_in) {
		kiv_os_rtl::Close_File_Descriptor(command.Get_Input_File_Descriptor());
	}
}

void Shell::Close_Command_Std_Out(const Command& command) const {
	if (command.Is_Output_File_Descriptor_Open() && command.Get_Output_File_Descriptor() != std_out) {
		kiv_os_rtl::Close_File_Descriptor(command.Get_Output_File_Descriptor());
	}
}

void Shell::Close_Command_Stdio(const Command& command) const {
	Close_Command_Std_In(command);
	Close_Command_Std_Out(command);
}

void Shell::Close_Command_List_Stdio(const std::vector<Command>& commands, const size_t count) const {
	for (size_t i = 0; i < count; i += 1) {
		Close_Command_Stdio(commands[i]);
	}
}

void Shell::Close_Command_List_Stdio(const std::vector<Command>& commands, const size_t start_idx,
                                     const size_t count) const {
	for (size_t i = start_idx; i < start_idx + count; i += 1) {
		Close_Command_Stdio(commands[i]);
	}
}


void Shell::Run() {
	// Zapiseme cwd
	Write(current_working_dir + ">");

	// Zpracovavame prikazy, dokud je co cist
	do {
		// Reset bufferu pro cteni
		std::fill_n(buffer.data(), buffer.size(), 0);

		// Cteme user input
		size_t bytes_read;
		if (const auto read_success = kiv_os_rtl::Read_File(std_in, buffer.data(), buffer.size(), bytes_read);
			!read_success) {
			// Nejde cist? ukoncime shell
			return;
		}

		// Pokud klavesnice zachyti ctrl c nebo ctrl d prestane cist dalsi data a preda je shellu
		// Tzn musime zkontrolovat, zda-li neni posledni znak control char a pripadne ho osetrit
		const auto last_char = bytes_read > 0 ? buffer[bytes_read - 1] : '\0';
		if (StringUtils::Is_Ctrl_C(last_char) || StringUtils::Is_Ctrl_D(last_char)) {
			Write_Line("Bye.");
			return;
		}

		// vstup z klavesnice
		auto keyboard_input = std::string(buffer.begin(),
		                                  bytes_read >= buffer.size()
			                                  ? buffer.end()
			                                  : buffer.begin() + bytes_read);

		// urizneme mezery zleva a zprava
		auto user_input = StringUtils::Trim_Whitespaces(keyboard_input);

		// Vytvorime seznam prikazu a zkusime je rozparsovat z uzivatelskeho vstupu
		auto commands = std::vector<Command>();
		try {
			Write_Line("");
			commands = command_parser.Parse_Commands(user_input);
		}
		catch (ParseException& ex) {
			// Pri chybe vypiseme hlasku do konzole a restartujeme while loop
			Write_Line(ex.what());
			continue;
		}

		Run_Commands(commands); // Provedeme vsechny prikazy
		if (run) {
			// pokud se nezavolal exit zobrazime cwd aby
			Write(current_working_dir + ">");
		}
	}
	while (run);

	// Konec pomoci Exitu
	Write_Line("Bye.");
}

std::pair<bool, std::string> Shell::Change_Directory(const Command& command) {

	// Zavolame sluzbu pro zmenu adresare
	const auto params = command.Get_Rtl_Params();
	if (const auto success = kiv_os_rtl::Set_Working_Dir(params); !success) {
		return {false, "Error, could not change directory for cd with arguments: " + params};
	}

	// Pokud zmena adresare probehla v poradku musime jeste aktualizovat stav v konzoli
	constexpr auto new_dir_buffer_size = BUFFER_SIZE / 2; // velikost bufferu pro zapsani vysledku
	auto new_directory_buffer = std::array<char, new_dir_buffer_size>(); // buffer pro zapsani vysledku
	auto new_directory_str_size = uint32_t{0};
	if (const auto success = kiv_os_rtl::Get_Working_Dir(new_directory_buffer.data(), new_dir_buffer_size,
	                                                     new_directory_str_size); !success) {
		Terminate();
		return {false, "Critical error ocurred, cannot get current working directory. Shell will close."};
	}

	current_working_dir = std::string(new_directory_buffer.data());
	return {true, ""};
}


void Shell::Run_Commands(std::vector<Command>& commands) {

	// Pokud doslo k chybe vypiseme ji a prikazy nespustime
	if (auto [success, err_message] = Prepare_Stdio_For_Commands(commands); !success) {
		Write_Line(err_message);
		return;
	}

	// Jinak zacneme vytvaret procesy pro kazdy prikaz
	auto program_pids = std::vector<kiv_os::THandle>();
	for (size_t i = 0; i < commands.size(); i += 1) {
		const auto& command = commands[i];

		// Shell neni mozne volat pres syscall, takze se pro nej musi vytvorit v shellu specialni funkce
		if (command.command_name == "cd") {
			if (const auto [success, errorMessage] = Change_Directory(command); !success) {
				Write_Line(errorMessage);
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
		const auto success = kiv_os_rtl::Create_Process(command.command_name, command.Get_Rtl_Params(),
		                                                command.Get_Input_File_Descriptor(),
		                                                command.Get_Output_File_Descriptor(), pid);

		if (!success) {
			switch (kiv_os_rtl::Get_Last_Err()) {
				case kiv_os::NOS_Error::Out_Of_Memory: {
					Write_Line("Error, OS does not have enough memory for next process.");
					break;
				}

				case kiv_os::NOS_Error::Invalid_Argument: {
					Write_Line("Error specified program: " + command.command_name + " does not exist!");
					break;
				}

				default: {
					Write_Line("An unknown OS error has occurred.");
					break;
				}
			}

			// Protoze nastala chyba az po tom co se pipy uspesne vytvorili musime je zavrit
			// Nicmene nemuzeme bezicim procesum pod rukama zavrit neco s cim pracuji a nechame je dobehnout
			// Zavreme pouze stdio pro od tohoto procesu do konce a stdout pro predchozi proces
			if (i > 0) {
				Close_Command_Std_Out(commands[i - 1]);
			}

			Close_Command_List_Stdio(commands, i, commands.size() - i);
			break;
		}

		program_pids.push_back(pid);
	}


	// Shell pocka na dokonceni vsech procesu
	for (const auto pid : program_pids) {
		const auto wait_for_list = {pid};
		kiv_os_rtl::Wait_For(wait_for_list);
		auto exit_code = kiv_os::NOS_Error::Success;
		// Precteme exit code pro odstraneni z tabulky
		kiv_os_rtl::Read_Exit_Code(pid, exit_code);
	}
}
