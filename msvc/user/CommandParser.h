#pragma once
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>
#include "StringUtils.h"

/**
 * Obsahuje data pro jeden prikaz
 */
struct Command {
	/*
	 * Parametry prikazu
	 */
	std::vector<std::string> params;

	/**
	 * Nazev prikazu
	 */
	std::string commandName;

	std::string inputIn;

	std::string inputOut;
};

class CommandParser {
	/**
	 * Vsechny dostupne programy v OS
	 */
	std::unordered_set<std::string> possiblePrograms = {
		"type", "md", "rd", "dir", "echo", "find", "sort", "rgen", "tasklist", "freq", "shutdown", "cd"
	};

	const std::regex whitespaceRegex = std::regex("\\s");

public:
	inline auto parseCommands(const std::string& input) {
		return StringUtils::splitByRegex(input, whitespaceRegex);

		// TODO
		auto commands = std::vector<Command>();
		
	}
};


