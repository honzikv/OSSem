#pragma once

#include <unordered_set>
#include <array>
#include "../../api/api.h"
#include "CommandParser.h"
#include "../rtl.h"
#include "../Utils/Logging.h"

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
	void Write_Line(const std::string& message) const;

	/// <summary>
	/// Ukonci shell - zavola se pro exit
	/// </summary>
	void Terminate();

	std::pair<bool, std::string> Prepare_Stdio_For_Commands(std::vector<Command>& commands) const;

	/// <summary>
	/// Zavre stdin pro prikaz
	/// </summary>
	/// <param name="command"></param>
	void Close_Command_Std_In(const Command& command) const;

	/// <summary>
	/// Zavre stdout pro prikaz
	/// </summary>
	/// <param name="command"></param>
	void Close_Command_Std_Out(const Command& command) const;

	/// <summary>
	/// Zavre stdio pro prikaz
	/// </summary>
	/// <param name="command"></param>
	void Close_Command_Stdio(const Command& command) const;

	/// <summary>
	/// Zavre stdio pro prikazy ve vektoru - od 0. indexu az do count (vcetne)
	/// </summary>
	/// <param name="commands">Reference na vektor</param>
	/// <param name="count">Pocet prikazu, pro ktere se ma stdio zavrit</param>
	void Close_Command_List_Stdio(const std::vector<Command>& commands, size_t count) const;

	/// <summary>
	/// Zavre stdio pro prikazy ve vektoru - od start_idx indexu az do start_idx + count (vcetne)
	/// </summary>
	/// <param name="commands">Reference na vektor</param>
	/// <param name="start_idx">Pocatecni index</param>
	/// <param name="count">Pocet prikazu, pro ktere se ma stdio zavrit</param>
	void Close_Command_List_Stdio(const std::vector<Command>& commands, size_t start_idx, size_t count) const;

	/// <summary>
	/// Pripravi stdio pro prave jeden prikaz (bez ||)
	/// </summary>
	/// <param name="command">Reference na prikaz</param>
	/// <returns></returns>
	std::pair<bool, std::string> Prepare_Stdio_For_Single_Command(Command& command) const;

	/// <summary>
	/// Pripravi pipe pro prvni prikaz
	/// </summary>
	/// <param name="command"></param>
	/// <param name="next_std_in"></param>
	/// <returns>Vysledek</returns>
	std::pair<bool, std::string> Prepare_Stdio_For_First_Command(Command& command, kiv_os::THandle& next_std_in) const;

	/// <summary>
	/// Provadi seznam prikazu, dokud nenastane chyba
	/// </summary>
	/// <param name="commands">Seznam prikazu, ktery se ma provest</param>
	void Run_Commands(std::vector<Command>& commands);

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
	[[nodiscard]] std::vector<Command> Parse_Commands(const std::string& line) const;
#endif

	/// <summary>
	/// Spusti shell - ten bezi, dokud se nezavola exit nebo shutdown
	/// </summary>
	void Run();

	/// <summary>
	/// CD prikaz
	/// </summary>
	/// <param name="command">reference na prikaz - pro ziskani argumentu</param>
	/// <returns></returns>
	std::pair<bool, std::string> Change_Directory(const Command& command);

};
