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
enum RedirectType : uint8_t {
	FromFile, // Ze souboru
	ToFile, // Do souboru
	None // Zadny - Default

};

/**
 * Obsahuje data pro jeden prikaz
 */
struct Command {
	/*
	 * Parametry prikazu
	 */
	const std::vector<std::string> params;

	/**
	 * Nazev prikazu
	 */
	const std::string commandName;

	/**
	 * Typ presmerovani - ze souboru, nebo do souboru
	 */
	const RedirectType redirectType;

	/**
	 * Reference na soubor pro presmerovani (pokud existuje)
	 */
	const std::string file;


	Command(std::string commandName, std::vector<std::string> params,
	        RedirectType redirectType = {}, std::string file = {});

	/**
	 * ToString pro debug
	 */
	[[nodiscard]] std::string toString() const;

	/**
	 * Operator pro rovnost (pouziti v unit testech)
	 */
	friend bool operator==(const Command& lhs, const Command& rhs);

	friend bool operator!=(const Command& lhs, const Command& rhs) { return !(lhs == rhs); }
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

	/**
	 * Funkce pro vytvoreni prikazu
	 */
	static auto createCommand(const std::vector<std::string>& commandWithParams, const RedirectType redirectType,
	                          const std::string& fileUri, const std::string& commandWithParamsAndRedirect) -> Command;
	/**
	 * Rozdeli soubory podle < nebo > a vrati je jako pair RedirectType a vektor tokenu
	 */
	[[nodiscard]]
	auto splitByFileRedirect(const std::string& commandWithParams) const
	-> std::pair<RedirectType, std::vector<std::string>> ;

public:
	/**
	 * Zpracuje vsechny prikazy, pokud je to mozne. Jinak vyhodi ParseException
	 */
	[[nodiscard]]
	auto parseCommands(const std::string& input) const -> std::vector<Command> ;
};
