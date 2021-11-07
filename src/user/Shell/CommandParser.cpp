#include "CommandParser.h"
#include <sstream>

Command::Command(std::string commandName, std::vector<std::string> params, std::string sourceFile,
	std::string targetFile):
	params(std::move(params)),
	commandName(std::move(commandName)),
	redirectType(resolveRedirectType(sourceFile, targetFile)),
	sourceFile(std::move(sourceFile)),
	targetFile(std::move(targetFile)) {

}



const char* ParseException::what() const throw() {
	return err.c_str();
}

Command CommandParser::createCommand(const std::vector<std::string>& commandWithParams, const RedirectType redirectType,
	const std::string& sourceFile, const std ::string& targetFile, const std::string& commandWithParamsAndRedirect)  {
	if (commandWithParams.empty()) {
		throw ParseException("Error, could not parse command file_name as it was not present.");
	}

	// Pokud neni redirect none, musi uri pro file byt neprazdny retezec
	if (redirectType != RedirectType::None) {

		// Vratime prikaz - pokud nema parametry, vytvorime pouze prazdny vektor, jinak je zkopirujeme z indexu 1..n
		return Command(commandWithParams[0],
		               commandWithParams.size() > 1
			               ? std::vector(commandWithParams.begin() + 1, commandWithParams.end())
			               : std::vector<std::string>(), sourceFile, targetFile);
	}

	// Jinak je prikaz bez redirectu -> file pro presmerovani bude prazdny retezec
	// Opet zkopirujeme z 1..n parametry, pokud existuji
	return Command(commandWithParams[0],
	               commandWithParams.size() > 1
		               ? std::vector(commandWithParams.begin() + 1, commandWithParams.end())
		               : std::vector<std::string>());
}

RedirectType CommandParser::getRedirectType(const std::string& commandWithParams) const {
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

	if (numberOfFromFileRedirects == 1 && numberOfToFileRedirects == 1) {
		return RedirectType::Both;
	}

	if (numberOfFromFileRedirects == 1) {
		return RedirectType::FromFile;
	}

	return numberOfToFileRedirects == 1 ? RedirectType::ToFile : RedirectType::None;
}

auto CommandParser::splitByFileRedirect(const std::string& commandWithParams, const RedirectType redirectType) const
-> std::tuple<std::string, std::string, std::string> {

	if (redirectType == RedirectType::None) {
		return { "", "", commandWithParams};
	}

	// Pokud nastane situace, ze mame oba dva redirecty, nejprve rozdelime podle symbolu <> a nasledne zpracujeme oba stringy
	if (redirectType == RedirectType::Both) {
		auto sourceFileIdx = commandWithParams.find_first_of('<');
		auto targetFileIdx = commandWithParams.find_first_of('>');
		auto targetFile = targetFileIdx > sourceFileIdx
			? commandWithParams.substr(targetFileIdx + 1, commandWithParams.size() - targetFileIdx)
			: commandWithParams.substr(targetFileIdx + 1, commandWithParams.size() - sourceFileIdx);
		auto sourceFile = sourceFileIdx > targetFileIdx
			? commandWithParams.substr(sourceFileIdx + 1, commandWithParams.size() - sourceFileIdx)
			: commandWithParams.substr(sourceFileIdx + 1, commandWithParams.size() - targetFileIdx);
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
			commandWithParams.substr(0, sourceFileIdx > targetFileIdx ? targetFileIdx : sourceFileIdx)
		};
	}
	
	// Pro jeden redirect muzeme snadno rozdelit pomoci regexu
	auto splitByRedirect = StringUtils::splitByRegex(commandWithParams, redirectRegex);

	// Ze splitu bysme meli dostat dvojici prikaz args (0ty prvek) a nazev souboru
		// Nicmene se muze stat ze se string nevyskytuje (napr. command >|), ale redirect symbol ano.
		// V tomto pripade vyhodime exception
	if (splitByRedirect.size() != 2) {
		const auto redirectSymbol = redirectType == RedirectType::FromFile ? "\"<\"" : "\">\"";
		throw ParseException(std::string(R"(Error, no file specified for redirect )") + redirectSymbol);
	}

	// Jinak string se souborem existuje a my ho muzeme upravit
	// Odstranime mezery zepredu a z konce
	const auto fileUri = StringUtils::trimWhitespaces(splitByRedirect[1]);

	// A pokud tim zbyde prazdny retezec vyhodime exception
	if (fileUri.empty()) {
		const auto redirectSymbol = redirectType == RedirectType::FromFile ? "\"<\"" : "\">\"";
		throw ParseException(std::string(R"(Error, no file specified for redirect )") + redirectSymbol);
	}

	if (redirectType == RedirectType::FromFile) {
		return { fileUri, "", commandWithParams };
	}

	return { "", fileUri, commandWithParams };
}

std::vector<Command> CommandParser::parseCommands(const std::string& input) const {
	if (input.empty() || input == "\n") {
		// Pokud se jedna o newline nebo je string prazdny, preskocime
		return std::vector<Command>();
	}

	auto result = std::vector<Command>();

	// Nejprve split pomoci pipe symbolu
	const auto tokensByPipeSymbol = StringUtils::splitByRegex(input, pipeRegex);

	for (const auto& splitByPipe : tokensByPipeSymbol) {
		auto commandWithParamsAndRedirect = StringUtils::trimWhitespaces(splitByPipe);
		if (commandWithParamsAndRedirect.empty()) {
			continue;
		}

		// Ziskame typ redirectu
		auto redirectType = getRedirectType(commandWithParamsAndRedirect);

		// Rozparsujeme string pro presmerovani
		auto [sourceFile, targetFile, splitByRedirectSymbols] = 
			splitByFileRedirect(commandWithParamsAndRedirect, redirectType);

		// Odstranime vsechny whitespaces na zacatku a konci - tzn. z " test " dostaneme "test"
		auto trimmed = StringUtils::trimWhitespaces(splitByRedirectSymbols);

		// Levou stranu od redirect symbolu rozdelime uz jenom podle mezer
		auto commandWithParams = StringUtils::splitByRegex(trimmed, whitespaceRegex);

		// Vytvorime prikaz
		auto command = createCommand(commandWithParams, redirectType, sourceFile, targetFile, commandWithParamsAndRedirect);
		result.push_back(command);
	}

	// Vratime vysledek
	return result;
}

bool operator==(const Command& lhs, const Command& rhs) {
	return lhs.params == rhs.params // == pro vector udela == pro kazdy element
		&& lhs.commandName == rhs.commandName
		&& lhs.redirectType == rhs.redirectType
		&& lhs.sourceFile == rhs.sourceFile
		&& lhs.targetFile == rhs.targetFile;
}
