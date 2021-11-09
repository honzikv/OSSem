#pragma once
#include <memory>
#include <unordered_set>
#include <array>
#include "../api/api.h"
#include "CommandParser.h"
#include "rtl.h"

constexpr auto NEWLINE_MESSAGE = "\n";
constexpr size_t BUFFER_SIZE = 512;
constexpr auto NEWLINE_SYMBOL = "\n";
constexpr auto EXIT_COMMAND = "exit";


// Debug
#define IS_DEBUG true

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters& regs);

/// <summary>
/// Pro prehlednost je shell objekt
/// </summary>
class Shell {

	/// <summary>
	/// Reference na registry
	/// </summary>
	kiv_hal::TRegisters registers;

	/// <summary>
	/// Reference na standardni vstup a vystup
	/// </summary>
	const kiv_os::THandle std_in, std_out;

	/// <summary>
	/// Interni objekt na parsovani dat
	/// </summary>
	const std::unique_ptr<CommandParser> command_parser = std::make_unique<CommandParser>();

	/// <summary>
	/// Aktualni cesta, ve ktere se shell nachazi
	/// </summary>
	std::string current_path;

	/// <summary>
	/// Buffer na IO
	/// </summary>
	std::array<char, BUFFER_SIZE> buffer = {};

	void Write(const std::string& message) const;

	void WriteLine(const std::string& message) const;


public:
	/// <summary>
	/// Konstruktor
	/// </summary>
	/// <param name="registers">Registry</param>
	/// <param name="std_in">Standardni vstup</param>
	/// <param name="std_out">Standardni vystup</param>
	/// <param name="current_path">Aktualni cesta</param>
	Shell(const kiv_hal::TRegisters& registers, kiv_os::THandle std_in, kiv_os::THandle std_out,
	      const std::string& current_path);

#if IS_DEBUG
	std::vector<Command> ParseCommands(const std::string& line);
#endif

	void ExecuteCommands(const std::vector<Command>& commands) {
		
	}

	void Run() {
		auto command_parser = CommandParser();
		while (strcmp(buffer.data(), EXIT_COMMAND) != 0) {
			Write(current_path); // Zapiseme aktualni cestu

			// Uzivatelsky vstup
			size_t bytesRead;
			if (kiv_os_rtl::Read_File(std_in, buffer.data(), buffer.size(), bytesRead)) {
				if (bytesRead < buffer.size()) { }

				// Ziskame uzivatelsky vstup
				auto user_input = std::string(buffer.begin(),
				                              bytesRead >= buffer.size()
					                              ? buffer.end()
					                              : buffer.begin() + bytesRead);

				auto commands = std::vector<Command>();
				try {
					commands = command_parser.ParseCommands(user_input);
					WriteLine("");
				}
				catch (ParseException& ex) { // Pri chybe vypiseme hlasku do konzole a restartujeme while loop
					WriteLine(ex.what());
					continue;
				}

				ExecuteCommands(commands); // Provedeme vsechny prikazy
			}
			else {
				break;
			}
		}
	}


};
