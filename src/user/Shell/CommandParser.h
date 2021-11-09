#pragma once
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include "Utils/StringUtils.h"

/**
 * Typ presmerovani v souboru
 */
enum class RedirectType : uint8_t {
	FromFile,
	// Ze souboru
	ToFile,
	// Do souboru
	Both,
	// Ze souboru i do souboru
	None // Zadny - Default
};

/// <summary>
/// Reprezentuje prikaz v commandline
/// </summary>
struct Command {
	/// <summary>
	/// Parametry prikazu
	/// </summary>
	const std::vector<std::string> params;

	/// <summary>
	/// Jmeno pozadovaneho prikazu
	/// </summary>
	const std::string command_name;

	/// <summary>
	/// Typ presmerovani - ze souboru, do souboru, oboji, zadne
	/// </summary>
	const RedirectType redirect_type;

	/// <summary>
	/// Vstupni soubor (napr. xyz < source.json)
	/// </summary>
	const std::string source_file;

	/// <summary>
	/// Cilovy soubor (napr. xyz > target.json)
	/// </summary>
	const std::string target_file;

	Command(std::string command_name, std::vector<std::string> params, std::string source_file = "",
	        std::string target_file = "");

	[[nodiscard]] std::string ToString() const;

	friend bool operator==(const Command& lhs, const Command& rhs);

	friend bool operator!=(const Command& lhs, const Command& rhs) { return !(lhs == rhs); }


private:
	static inline RedirectType ResolveRedirectType(const std::string& source_file, const std::string& target_file) {
		if (source_file.empty() && target_file.empty()) {
			return RedirectType::None;
		}

		if (!source_file.empty() && !target_file.empty()) {
			return RedirectType::Both;
		}

		return !source_file.empty() ? RedirectType::FromFile : RedirectType::ToFile;
	}
};

/**
 * Exception pri parsovani uzivatelskeho vstupu
 */
class ParseException final : public std::exception {

	/**
	 * Chybova hlaska
	 */
	std::string err;

public:
	explicit ParseException(std::string what) : err(std::move(what)) {}

	[[nodiscard]] const char* what() const throw() override;
};

class CommandParser {

	const std::regex PIPE_REGEX = std::regex("\\|");
	const std::regex REDIRECT_TO_FILE_REGEX = std::regex("\\>");
	const std::regex REDIRECT_FROM_FILE_REGEX = std::regex("\\<");
	const std::regex REDIRECT_REGEX = std::regex("\\<|\\>");
	const std::regex WHITESPACE_REGEX = std::regex("\\s+");

	static Command CreateCommand(const std::vector<std::string>& command_with_params,
	                             const RedirectType redirect_type, const std::string& source_file,
	                             const std::string& target_file,
	                             const std::string& command_with_params_and_redirect);

	/// <summary>
	/// Rozdeli radek obsahujici prikaz, argumenty a (potencialni) redirecty souboru na trojici:
	/// zdrojovy soubor, cilovy soubor, prikaz s argumenty
	/// </summary>
	/// <param name="command_with_params">Prikaz s argumenty a presmerovanim</param>
	/// <param name="redirect_type">Typ presmerovani (z metody getRedirectType)</param>
	/// <returns>zdrojovy soubor, cilovy soubor, prikaz s argumenty</returns>
	[[nodiscard]] auto SplitByFileRedirect(const std::string& command_with_params,
	                                       const RedirectType redirect_type) const
	-> std::tuple<std::string, std::string, std::string>;


	/// <summary>
	/// Zjisti typ presmerovani v prikazu
	/// </summary>
	/// <param name="command_with_params">Prikaz s argumenty a presmerovanim</param>
	/// <returns>Typ presmerovani</returns>
	[[nodiscard]] RedirectType GetRedirectType(const std::string& command_with_params) const;

public:
	/// <summary>
	/// Zpracuje vsechny prikazy z jedne radky a vrati vektor prikazu, ktere lze dale zpracovat
	/// </summary>
	/// <param name="input">Radka </param>
	/// <returns>Typ presmerovani</returns>
	[[nodiscard]] std::vector<Command> ParseCommands(const std::string& input) const;
};
