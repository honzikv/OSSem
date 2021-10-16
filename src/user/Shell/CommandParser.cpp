#include "CommandParser.h"
#include <sstream>

Command::Command(std::string commandName, std::vector<std::string> params, const RedirectType redirectType,
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
