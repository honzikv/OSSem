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
	FromFile, // Ze souboru
	ToFile, // Do souboru
	Both, // Ze souboru i do souboru
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
	const std::string commandName;

	/// <summary>
	/// Typ presmerovani - ze souboru, do souboru, oboji, zadne
	/// </summary>
	const RedirectType redirectType;

	/// <summary>
	/// Vstupni soubor (napr. xyz < source.json)
	/// </summary>
	const std::string sourceFile;

	/// <summary>
	/// Cilovy soubor (napr. xyz > target.json)
	/// </summary>
	const std::string targetFile;

	Command(std::string commandName, std::vector<std::string> params, std::string sourceFile = "", std::string targetFile = "");


	[[nodiscard]] std::string toString() const;
	
	friend bool operator==(const Command& lhs, const Command& rhs);

	friend bool operator!=(const Command& lhs, const Command& rhs) { return !(lhs == rhs); }

private:
	static inline RedirectType resolveRedirectType(const std::string& sourceFile, const std::string& targetFile) {
		if (sourceFile.empty() && targetFile.empty()) {
			return RedirectType::None;
		}

		if (!sourceFile.empty() && !targetFile.empty()) {
			return RedirectType::Both;
		}

		return !sourceFile.empty() ? RedirectType::FromFile : RedirectType::ToFile;
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

	const std::regex pipeRegex = std::regex("\\|");
	const std::regex redirectToFileRegex = std::regex("\\>");
	const std::regex redirectFromFileRegex = std::regex("\\<");
	const std::regex redirectRegex = std::regex("\\<|\\>");
	const std::regex whitespaceRegex = std::regex("\\s+");

	
	static Command createCommand(const std::vector<std::string>& commandWithParams, 
	                             const RedirectType redirectType, const std::string& sourceFile, const std::string& targetFile, 
	                             const std::string& commandWithParamsAndRedirect);

	
	
	/// <summary>
	/// Rozdeli radek obsahujici prikaz, argumenty a (potencialni) redirecty souboru na trojici:
	/// zdrojovy soubor, cilovy soubor, prikaz s argumenty
	/// </summary>
	/// <param name="commandWithParams">Prikaz s argumenty a presmerovanim</param>
	/// <param name="redirectType">Typ presmerovani (z metody getRedirectType)</param>
	/// <returns>zdrojovy soubor, cilovy soubor, prikaz s argumenty</returns>
	[[nodiscard]] auto splitByFileRedirect(const std::string& commandWithParams, const RedirectType redirectType) const
	-> std::tuple<std::string, std::string, std::string>;

	
	
	/// <summary>
	/// Zjisti typ presmerovani v prikazu
	/// </summary>
	/// <param name="commandWithParams">Prikaz s argumenty a presmerovanim</param>
	/// <returns>Typ presmerovani</returns>
	[[nodiscard]] RedirectType getRedirectType(const std::string& commandWithParams) const;

public:

	/// <summary>
	/// Zpracuje vsechny prikazy z jedne radky a vrati vektor prikazu, ktere lze dale zpracovat
	/// </summary>
	/// <param name="input">Radka </param>
	/// <returns>Typ presmerovani</returns>
	[[nodiscard]] std::vector<Command> parseCommands(const std::string& input) const;
};
