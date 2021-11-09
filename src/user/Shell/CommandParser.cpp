#include "CommandParser.h"
#include <sstream>

Command::Command(std::string command_name, std::vector<std::string> params, std::string source_file,
                 std::string target_file):
	params(std::move(params)),
	command_name(std::move(command_name)),
	redirect_type(ResolveRedirectType(source_file, target_file)),
	source_file(std::move(source_file)),
	target_file(std::move(target_file)) {}

std::string Command::ToString() const {

	std::string redirectTypeStr;
	switch (redirect_type) {
		case RedirectType::Both:
			redirectTypeStr = "BOTH";
			break;
		case RedirectType::FromFile:
			redirectTypeStr = "FROM_FILE";
			break;
		case RedirectType::None:
			redirectTypeStr = "NONE";
			break;
		case RedirectType::ToFile:
			redirectTypeStr = "TO_FILE";
			break;
	}

	auto stringStream = std::stringstream();
	stringStream << "commandName: " << command_name << ", redirectType: " << redirectTypeStr << ", Sourcefile: " <<
		source_file << ", targetFile: " << target_file << std::endl;
	stringStream << "params: [";
	for (const auto& param : params) {
		stringStream << param << " ";
	}
	stringStream << "]";
	return stringStream.str();
}


const char* ParseException::what() const throw() {
	return err.c_str();
}

Command CommandParser::CreateCommand(const std::vector<std::string>& command_with_params, const RedirectType redirect_type,
                                     const std::string& source_file, const std::string& target_file,
                                     const std::string& command_with_params_and_redirect) {
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

RedirectType CommandParser::GetRedirectType(const std::string& command_with_params) const {
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

auto CommandParser::SplitByFileRedirect(const std::string& command_with_params, const RedirectType redirect_type) const
-> std::tuple<std::string, std::string, std::string> {

	if (redirect_type == RedirectType::None) {
		return {"", "", command_with_params};
	}

	// Pokud nastane situace, ze mame oba dva redirecty, nejprve rozdelime podle symbolu <> a nasledne zpracujeme oba stringy
	if (redirect_type == RedirectType::Both) {
		auto sourceFileIdx = command_with_params.find_first_of('<');
		auto targetFileIdx = command_with_params.find_first_of('>');
		auto targetFile = targetFileIdx > sourceFileIdx
			                  ? command_with_params.substr(targetFileIdx + 1, command_with_params.size() - targetFileIdx)
			                  : command_with_params.substr(targetFileIdx + 1, command_with_params.size() - sourceFileIdx);
		auto sourceFile = sourceFileIdx > targetFileIdx
			                  ? command_with_params.substr(sourceFileIdx + 1, command_with_params.size() - sourceFileIdx)
			                  : command_with_params.substr(sourceFileIdx + 1, command_with_params.size() - targetFileIdx);
		targetFile = StringUtils::trimWhitespaces(targetFile);
		sourceFile = StringUtils::trimWhitespaces(sourceFile);

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
	auto splitByRedirect = StringUtils::splitByRegex(command_with_params, REDIRECT_REGEX);

	// Ze splitu bysme meli dostat dvojici prikaz args (0ty prvek) a nazev souboru
	// Nicmene se muze stat ze se string nevyskytuje (napr. command >|), ale redirect symbol ano.
	// V tomto pripade vyhodime exception
	if (splitByRedirect.size() != 2) {
		const auto redirectSymbol = redirect_type == RedirectType::FromFile ? "\"<\"" : "\">\"";
		throw ParseException(std::string(R"(Error, no file specified for redirect )") + redirectSymbol);
	}

	// Jinak string se souborem existuje a my ho muzeme upravit
	// Odstranime mezery zepredu a z konce
	const auto fileUri = StringUtils::trimWhitespaces(splitByRedirect[1]);

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

std::vector<Command> CommandParser::ParseCommands(const std::string& input) const {
	if (input.empty() || input == "\n") {
		// Pokud se jedna o newline nebo je string prazdny, preskocime
		return std::vector<Command>();
	}

	auto result = std::vector<Command>();

	// Nejprve split pomoci pipe symbolu
	const auto tokensByPipeSymbol = StringUtils::splitByRegex(input, PIPE_REGEX);

	for (const auto& splitByPipe : tokensByPipeSymbol) {
		auto commandWithParamsAndRedirect = StringUtils::trimWhitespaces(splitByPipe);
		if (commandWithParamsAndRedirect.empty()) {
			continue;
		}

		// Ziskame typ redirectu
		auto redirectType = GetRedirectType(commandWithParamsAndRedirect);

		// Rozparsujeme string pro presmerovani
		auto [sourceFile, targetFile, splitByRedirectSymbols] =
			SplitByFileRedirect(commandWithParamsAndRedirect, redirectType);

		// Odstranime vsechny whitespaces na zacatku a konci - tzn. z " test " dostaneme "test"
		auto trimmed = StringUtils::trimWhitespaces(splitByRedirectSymbols);

		// Levou stranu od redirect symbolu rozdelime uz jenom podle mezer
		auto commandWithParams = StringUtils::splitByRegex(trimmed, WHITESPACE_REGEX);

		// Vytvorime prikaz
		auto command = CreateCommand(commandWithParams, redirectType, sourceFile, targetFile,
		                             commandWithParamsAndRedirect);
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
