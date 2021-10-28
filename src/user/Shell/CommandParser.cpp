#include "CommandParser.h"
#include <sstream>

Command::Command(std::string commandName, std::vector<std::string> params, RedirectType redirectType,
                 std::string file):
	params(std::move(params)),
	commandName(std::move(commandName)),
	redirectType(redirectType),
	file(std::move(file)) { }

std::string Command::toString() const {
	auto stringStream = std::stringstream();
	stringStream << "commandName: " << commandName << ", redirectType: " << redirectType << ", file: " << file <<
		std::endl;
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

auto CommandParser::createCommand(const std::vector<std::string>& commandWithParams, const RedirectType redirectType,
                                  const std::string& fileUri,
                                  const std::string& commandWithParamsAndRedirect) -> Command {
	if (commandWithParams.empty()) {
		throw ParseException("Error, could not parse command file_name as it was not present."); // TODO asi se nestane
	}

	// Pokud neni redirect none, musi uri pro file byt neprazdny retezec
	if (redirectType != RedirectType::None) {

		// Vratime prikaz - pokud nema parametry, vytvorime pouze prazdny vektor, jinak je zkopirujeme z indexu 1..n
		return Command(commandWithParams[0],
		               commandWithParams.size() > 1
			               ? std::vector(commandWithParams.begin() + 1, commandWithParams.end())
			               : std::vector<std::string>(),
		               redirectType, fileUri);
	}

	// Jinak je prikaz bez redirectu -> file pro presmerovani bude prazdny retezec
	// Opet zkopirujeme z 1..n parametry, pokud existuji
	return Command(commandWithParams[0],
	               commandWithParams.size() > 1
		               ? std::vector(commandWithParams.begin() + 1, commandWithParams.end())
		               : std::vector<std::string>(),
	               redirectType, {});
}

auto CommandParser::splitByFileRedirect(
	const std::string& commandWithParams) const -> std::pair<RedirectType, std::vector<std::string>> {
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

	if (isFromFileRedirect || isToFileRedirect) {
		// Ze splitu bysme meli dostat dvojici prikaz args (0ty prvek) a nazev souboru
		// Nicmene se muze stat ze se string nevyskytuje (napr. command >|), ale redirect symbol ano.
		// V tomto pripade vyhodime exception
		if (splitByRedirect.size() != 2) {
			const auto redirectSymbol = isFromFileRedirect ? "\"<\"" : "\">\"";
			throw ParseException(std::string("Error, file redirect symbol ") + redirectSymbol
				+ " present but no file was specified for command: " + "\"" + commandWithParams + "\"");
		}

		// Jinak string se souborem existuje a my ho muzeme upravit
		// Odstranime mezery zepredu a z konce
		const auto trimmedFileUri = StringUtils::trimWhitespaces(splitByRedirect[1]);

		// A pokud tim zbyde prazdny retezec vyhodime exception
		if (trimmedFileUri.empty()) {
			const auto redirectSymbol = isFromFileRedirect ? "\"<\"" : "\">\"";
			throw ParseException(std::string("Error, file redirect symbol ") + redirectSymbol
				+ " present but no file was specified for command: " + "\"" + commandWithParams + "\"");
		}

		// Jinak nastavime file jako druhy item
		splitByRedirect[1] = trimmedFileUri;
	}

	// Vratime RedirectType a vector s prvnim prvkem prikaz + argumenty, a druhy nazev souboru (pokud existuje)
	return std::make_pair(redirectType, splitByRedirect);
}

auto CommandParser::parseCommands(const std::string& input) const -> std::vector<Command> {
	if (input.empty() || input == "\n") {
		// Pokud se jedna o newline nebo je string prazdny, preskocime
		return std::vector<Command>();
	}

	auto result = std::vector<Command>();

	// Nejprve split pomoci pipe symbolu
	const auto tokensByPipeSymbol = StringUtils::splitByRegex(input, pipeRegex);

	for (const auto& commandWithParamsAndRedirect : tokensByPipeSymbol) {
		if (commandWithParamsAndRedirect.empty()) {
			continue;
		}

		// Dale rozdelime kazdy prikaz s parametry podle redirect symbolu (">" nebo "<")
		auto [redirectType, splitByRedirectSymbol] = splitByFileRedirect(commandWithParamsAndRedirect);

		// Odstranime vsechny whitespaces na zacatku a konci - tzn. z " test " dostaneme "test"
		auto trimmed = StringUtils::trimWhitespaces(splitByRedirectSymbol[0]);

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

bool operator==(const Command& lhs, const Command& rhs) {
	return lhs.params == rhs.params // == pro vector udela == pro kazdy element
		&& lhs.commandName == rhs.commandName
		&& lhs.redirectType == rhs.redirectType
		&& lhs.file == rhs.file;
}
