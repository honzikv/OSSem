#pragma once

#include <unordered_set>
#include <array>
#include "../api/api.h"
#include "CommandParser.h"
#include "rtl.h"
#include "Utils/Logging.h"

constexpr size_t BUFFER_SIZE = 512;
constexpr auto NEWLINE_SYMBOL = "\n";


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
	/// Instance command parseru
	/// </summary>
	CommandParser command_parser;

	/// <summary>
	/// Aktualni cesta, ve ktere se shell nachazi
	/// </summary>
	std::string current_working_dir;

	/// <summary>
	/// Buffer na IO
	/// </summary>
	std::array<char, BUFFER_SIZE> buffer = {};

	/// <summary>
	/// Flag pro beh konzole
	/// </summary>
	bool run = true;

	/// <summary>
	/// Zapise do konzole zpravu bez newline
	/// </summary>
	/// <param name="message">Reference na zpravu</param>
	void Write(const std::string& message) const;

	/// <summary>
	/// Zapise do konzole zpravu a prida jako posledni znak newline
	/// </summary>
	/// <param name="message">Reference na zpravu</param>
	void WriteLine(const std::string& message) const;

	/// <summary>
	/// Ukonci shell - zavola se pro exit
	/// </summary>
	void Terminate() {
		run = false;
	}

	std::pair<bool, std::string> PreparePipes(std::vector<Command>& commands) const;

	/// <summary>
	/// Zavre file descriptory pro dany prikaz. Pokud jsou file descriptory pro std_in a std_out shellu, metoda je ignoruje
	/// </summary>
	/// <param name="command">Reference na prikaz</param>
	void CloseCommandFileDescriptors(const Command& command) const;

	/// <summary>
	/// Zavre file descriptory pro dany pocet prikazu z vektoru. Pokud je pocet -1 zavre file descriptory pro vsechny prikazy
	/// </summary>
	/// <param name="commands">Reference na vektor s prikazy</param>
	/// <param name="count">Pocet prvku seznamu, pro ktery se maji file descriptory zavrit</param>
	void CloseCommandListFileDescriptors(const std::vector<Command>& commands, size_t count = -1) const;

	void CloseCommandListFileDescriptors(const std::vector<Command>& commands, size_t idx_start, size_t idx_end) const;

	std::pair<bool, std::string> PreparePipeForSingleCommand(Command& command) const;

	/// <summary>
	/// Pripravi pipe pro prvni prikaz
	/// </summary>
	/// <param name="command"></param>
	/// <returns>Vysledek</returns>
	std::pair<bool, std::string> PreparePipeForFirstCommand(Command& command, bool is_next_command = false) const;

	/// <summary>
	/// Pripravi pipe pro posledni prikaz
	/// </summary>
	/// <param name="second_last">Prikaz, ze ktereho se cte vstup</param>
	/// <param name="last">Posledni prikaz</param>
	/// <returns>Vysledek</returns>
	static std::pair<bool, std::string> PreparePipeForLastCommand(const Command& second_last, Command& last);


	/// <summary>
	/// Provadi seznam prikazu, dokud nenastane chyba
	/// </summary>
	/// <param name="commands">Seznam prikazu, ktery se ma provest</param>
	void RunCommands(std::vector<Command>& commands);

public:
	/// <summary>
	/// Konstruktor
	/// </summary>
	/// <param name="registers">Registry</param>
	/// <param name="std_in">Standardni vstup</param>
	/// <param name="std_out">Standardni vystup</param>
	/// <param name="current_path">Aktualni cesta</param>
	Shell(const kiv_hal::TRegisters& registers, kiv_os::THandle std_in, kiv_os::THandle std_out,
	      std::string current_path);

#if IS_DEBUG
	[[nodiscard]] std::vector<Command> ParseCommands(const std::string& line) const;
#endif


	/// <summary>
	/// Spusti shell - ten bezi, dokud se nezavola exit nebo shutdown
	/// </summary>
	void Run();
	
	std::pair<bool, std::string> ChangeDirectory(const Command& command);

};
