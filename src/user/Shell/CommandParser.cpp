#include "CommandParser.h"
#include <sstream>

const char* ParseException::what() const throw() {
	return err.c_str();
}

Command CommandParser::Create_Command(const std::vector<std::string>& command_with_params, const RedirectType redirect_type,
                                     const std::string& source_file, const std::string& target_file) {
	if (command_with_params.empty()) {
		throw ParseException("Error, could not parse command file_name as it was not present.");
	}

	// Pokud neni redirect none, musi uri pro file byt neprazdny retezec
	if (redirect_type != RedirectType::None) {

		// Vratime prikaz - pokud nema parametry, vytvorime pouze prazdny vektor, jinak je zkopirujeme z indexu 1..n
		return Command(command_with_params[0],
		               command_with_params.size() > 1
			               ? std::vector(command_with_params.begin() + 1, command_with_params.end())
			               : std::vector<std::string>(), source_file, target_file);
	}

	// Jinak je prikaz bez redirectu -> file pro presmerovani bude prazdny retezec
	// Opet zkopirujeme z 1..n parametry, pokud existuji
	return Command(command_with_params[0],
	               command_with_params.size() > 1
		               ? std::vector(command_with_params.begin() + 1, command_with_params.end())
		               : std::vector<std::string>());
}

RedirectType CommandParser::Get_Redirect_Type(const std::string& command_with_params) const {
	// Pocet symbolu ">" v prikazu
	const auto numberOfToFileRedirects =
		std::count(command_with_params.begin(), command_with_params.end(), '>');

	// Pocet symbolu "<" v prikazu
	const auto numberOfFromFileRedirects =
		std::count(command_with_params.begin(), command_with_params.end(), '<');

	// Pokud je pocet vyskytu symbolu pro presmerovani vetsi nebo rovno 2 vyhodime exception
	if (numberOfToFileRedirects >= 2) {
		throw ParseException(R"(Error, more than one redirect symbol ">" is present in: )" +
			command_with_params);
	}

	if (numberOfFromFileRedirects >= 2) {
		throw ParseException(R"(Error, more than one redirect symbol "<" is present in: )" +
			command_with_params);
	}

	if (numberOfFromFileRedirects == 1 && numberOfToFileRedirects == 1) {
		return RedirectType::Both;
	}

	if (numberOfFromFileRedirects == 1) {
		return RedirectType::FromFile;
	}

	return numberOfToFileRedirects == 1 ? RedirectType::ToFile : RedirectType::None;
}

auto CommandParser::Split_By_File_Redirect(const std::string& command_with_params, const RedirectType redirect_type) const
-> std::tuple<std::string, std::string, std::string> {

	if (redirect_type == RedirectType::None) {
		return {"", "", command_with_params};
	}

	// Pokud nastane situace, ze mame oba dva redirecty, nejprve rozdelime podle symbolu <> a nasledne zpracujeme oba stringy
	if (redirect_type == RedirectType::Both) {
		const auto sourceFileIdx = command_with_params.find_first_of('<');
		const auto targetFileIdx = command_with_params.find_first_of('>');
		auto targetFile = targetFileIdx > sourceFileIdx
			                  ? command_with_params.substr(targetFileIdx + 1, command_with_params.size() - targetFileIdx)
			                  : command_with_params.substr(targetFileIdx + 1, command_with_params.size() - sourceFileIdx);
		auto sourceFile = sourceFileIdx > targetFileIdx
			                  ? command_with_params.substr(sourceFileIdx + 1, command_with_params.size() - sourceFileIdx)
			                  : command_with_params.substr(sourceFileIdx + 1, command_with_params.size() - targetFileIdx);
		targetFile = StringUtils::Trim_Whitespaces(targetFile);
		sourceFile = StringUtils::Trim_Whitespaces(sourceFile);

		if (targetFile.empty()) {
			throw ParseException(R"(Error, no file specified for redirect "<")");
		}

		if (sourceFile.empty()) {
			throw ParseException(R"(Error, no file specified for redirect ">")");
		}

		return {
			sourceFile,
			targetFile,
			command_with_params.substr(0, sourceFileIdx > targetFileIdx ? targetFileIdx : sourceFileIdx)
		};
	}

	// Pro jeden redirect muzeme snadno rozdelit pomoci regexu
	auto splitByRedirect = StringUtils::Split_By_Regex(command_with_params, REDIRECT_REGEX);

	// Ze splitu bysme meli dostat dvojici prikaz args (0ty prvek) a nazev souboru
	// Nicmene se muze stat ze se string nevyskytuje (napr. command >|), ale redirect symbol ano.
	// V tomto pripade vyhodime exception
	if (splitByRedirect.size() != 2) {
		const auto redirectSymbol = redirect_type == RedirectType::FromFile ? "\"<\"" : "\">\"";
		throw ParseException(std::string(R"(Error, no file specified for redirect )") + redirectSymbol);
	}

	// Jinak string se souborem existuje a my ho muzeme upravit
	// Odstranime mezery zepredu a z konce
	const auto fileUri = StringUtils::Trim_Whitespaces(splitByRedirect[1]);

	// A pokud tim zbyde prazdny retezec vyhodime exception
	if (fileUri.empty()) {
		const auto redirectSymbol = redirect_type == RedirectType::FromFile ? "\"<\"" : "\">\"";
		throw ParseException(std::string(R"(Error, no file specified for redirect )") + redirectSymbol);
	}

	if (redirect_type == RedirectType::FromFile) {
		return {fileUri, "", splitByRedirect[0]};
	}

	return {"", fileUri, splitByRedirect[0]};
}

std::vector<Command> CommandParser::Parse_Commands(const std::string& input) const {
	if (input.empty() || input == "\n") {
		// Pokud se jedna o newline nebo je string prazdny, preskocime
		return std::vector<Command>();
	}

	auto result = std::vector<Command>();

	// Nejprve split pomoci pipe symbolu
	const auto num_pipes = std::count(input.begin(), input.end(), '|');
	const auto split_by_pipe_regex = StringUtils::Split_By_Regex(input, PIPE_REGEX);
	const auto tokens_by_pipe_symbol = StringUtils::FilterEmptyStrings(split_by_pipe_regex);

	if (tokens_by_pipe_symbol.size() - 1 != num_pipes) {
		throw ParseException("Invalid number of pipe symbols is present.");
	}

	for (const auto& split_by_pipe : tokens_by_pipe_symbol) {
		auto commandWithParamsAndRedirect = StringUtils::Trim_Whitespaces(split_by_pipe);
		if (commandWithParamsAndRedirect.empty()) {
			continue;
		}

		// Ziskame typ redirectu
		auto redirectType = Get_Redirect_Type(commandWithParamsAndRedirect);

		// Rozparsujeme string pro presmerovani
		auto [sourceFile, targetFile, splitByRedirectSymbols] =
			Split_By_File_Redirect(commandWithParamsAndRedirect, redirectType);

		// Odstranime vsechny whitespaces na zacatku a konci - tzn. z " test " dostaneme "test"
		auto trimmed = StringUtils::Trim_Whitespaces(splitByRedirectSymbols);

		// Levou stranu od redirect symbolu rozdelime uz jenom podle mezer
		auto commandWithParams = StringUtils::Split_By_Regex(trimmed, WHITESPACE_REGEX);

		// Vytvorime prikaz
		auto command = Create_Command(commandWithParams, redirectType, sourceFile, targetFile);
		result.push_back(command);
	}

	// Vratime vysledek
	return result;
}

bool operator==(const Command& lhs, const Command& rhs) {
	return lhs.params == rhs.params // == pro vector udela == pro kazdy element
		&& lhs.command_name == rhs.command_name
		&& lhs.redirect_type == rhs.redirect_type
		&& lhs.source_file == rhs.source_file
		&& lhs.target_file == rhs.target_file;
}
