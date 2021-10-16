#pragma once
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "Utils/StringUtils.h"

enum RedirectType : uint8_t {
	FromFile,
	ToFile,
	None

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
	        const RedirectType redirectType = {}, std::string file = {});

	// metoda pro debug
	[[nodiscard]] std::string toString() const;

	friend bool operator==(const Command& lhs, const Command& rhs) {
		return lhs.params == rhs.params // == pro vector udela == pro kazdy element
			&& lhs.commandName == rhs.commandName
			&& lhs.redirectType == rhs.redirectType
			&& lhs.file == rhs.file;
	}

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

	static auto createCommand(const std::vector<std::string>& commandWithParams, const RedirectType redirectType,
	                          const std::string& fileUri, const std::string& commandWithParamsAndRedirect) {
		if (commandWithParams.empty()) {
			throw ParseException("Error, could not parse command name as it was not present."); // TODO asi se nestane
		}

		// Pokud neni redirect none, musi uri pro file byt neprazdny retezec
		if (redirectType != RedirectType::None) {
			const auto trimmedFileUri = StringUtils::trimWhitespaces(fileUri);
			if (trimmedFileUri.empty()) {
				throw ParseException("Error, no file specified for command: " + commandWithParamsAndRedirect);
			}

			// Vratime prikaz - pokud nema parametry, vytvorime pouze prazdny vektor, jinak je zkopirujeme z indexu 1..n
			return Command(commandWithParams[0], 
				commandWithParams.size() > 1 
				? std::vector(commandWithParams.begin() + 1, commandWithParams.end())
				: std::vector<std::string>(), 
				redirectType, trimmedFileUri);
		}

		// Jinak je prikaz bez redirectu -> file pro presmerovani bude prazdny retezec
		// Opet zkopirujeme z 1..n parametry, pokud existuji
		return Command(commandWithParams[0],
			commandWithParams.size() > 1 
			? std::vector(commandWithParams.begin() + 1, commandWithParams.end())
			: std::vector<std::string>(), 
			redirectType, {});
	}

	[[nodiscard]]
	auto splitByFileRedirect(const std::string& commandWithParams) const {
		auto redirectType = RedirectType::None;

		// Pocet symbolu ">" v prikazu
		const auto numberOfToFileRedirects = 
			std::count(commandWithParams.begin(), commandWithParams.end(), '>');

		// Pocet symbolu "<" v prikazu
		const auto numberOfFromFileRedirects = 
			std::count(commandWithParams.begin(), commandWithParams.end(), '<');

		// Pokud je pocet vyskytu symbolu pro presmerovani vetsi nebo rovno 2 vyhodime exception
		if (numberOfToFileRedirects >= 2) {
			throw ParseException(R"(Error, more than one redirect symbol ">" is present in: )" +
				commandWithParams);
		}

		if (numberOfFromFileRedirects >= 2) {
			throw ParseException(R"(Error, more than one redirect symbol "<" is present in: )" +
				commandWithParams);
		}

		const auto isFromFileRedirect = numberOfFromFileRedirects == 1;
		const auto isToFileRedirect = numberOfToFileRedirects == 1;

		if (isFromFileRedirect && isToFileRedirect) {
			throw ParseException(R"(Error, both ">" and "<" redirect symbols present in: )" +
				commandWithParams);
		}

		if (isFromFileRedirect) {
			redirectType = RedirectType::FromFile;
		}
		else if (isToFileRedirect) {
			redirectType = RedirectType::ToFile;
		}

		auto splitByRedirect = StringUtils::splitByRegex(commandWithParams, redirectRegex);

		// Vratime RedirectType a vector s prvnim prvkem prikaz + argumenty, a druhy nazev souboru
		return std::make_pair(redirectType, splitByRedirect);
	}

public:
	/**
	 * Zpracuje vsechny prikazy, pokud je to mozne. Jinak vyhodi ParseException
	 */
	[[nodiscard]]
	inline auto parseCommands(const std::string& input) const {
		if (input.empty() || input == "\n") {
			// Pokud se jedna o newline nebo je string prazdny, preskocime
			return std::vector<Command>();
		}

		auto result = std::vector<Command>();

		// Nejprve split pomoci pipe symbolu
		const auto tokensByPipeSymbol = StringUtils::splitByRegex(input, pipeRegex);

		for (const auto& commandWithParamsAndRedirect : tokensByPipeSymbol) {
			// Dale rozdelime kazdy prikaz s parametry podle redirect symbolu (">" nebo "<")
			auto [redirectType, splitByRedirectSymbol] = splitByFileRedirect(commandWithParamsAndRedirect);

			// Odstranime vsechny whitespaces na zacatku a konci - tzn. z " test " dostaneme "test"
			auto trimmed = StringUtils::trimFromLeft(splitByRedirectSymbol[0]);
			trimmed = StringUtils::trimFromRight(trimmed);

			// Levou stranu od redirect symbolu rozdelime uz jenom podle mezer
			auto commandWithParams = StringUtils::splitByRegex(trimmed, whitespaceRegex);

			// Ziskame fileUri, pokud neni redirect type None
			auto fileUri = redirectType == RedirectType::None ? "" : splitByRedirectSymbol[1];

			auto command = createCommand(commandWithParams, redirectType, fileUri, commandWithParamsAndRedirect);
			result.push_back(command);
		}

		// Vratime vysledek
		return result;
	}
};
