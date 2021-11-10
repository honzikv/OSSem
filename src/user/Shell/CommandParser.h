#pragma once
#include <regex>
#include <string>
#include <vector>

#include "Command.h"
#include "Utils/StringUtils.h"


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
