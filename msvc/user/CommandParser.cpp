#include "CommandParser.h"

Command::Command(std::string commandName, std::vector<std::string> params, const RedirectType redirectType,
	std::string file):
	params(std::move(params)),
	commandName(std::move(commandName)),
	redirectType(redirectType),
	file(std::move(file)) { }

const char* ParseException::what() const throw() {
	return err.c_str();
}
