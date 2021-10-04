#pragma once
#include <iostream>
#include <ostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CommandParser
{
	/**
	 * Vsechny dostupne programy v OS
	 */
	std::unordered_set<std::string> possiblePrograms = {
		"type", "md", "rd", "dir", "echo", "find", "sort", "rgen", "tasklist", "freq", "shutdown"
	};

	const std::regex splitRegex = std::regex("\\s+<>");

	inline std::string parseCommands(const std::string& input)
	{
		auto result = std::vector<std::string>();
	}

	// Rozdeleni podle " ", |, >, <
	inline auto split(const std::string& input)
	{
		auto itemIterator = std::sregex_token_iterator(input.begin(), input.end(), splitRegex, -1);
		auto end = std::sregex_token_iterator();

		auto items = std::vector<std::string>();
		while (itemIterator != end)
		{
			items.push_back(*itemIterator);
		}

		return items;
	}
public:
	inline auto parseCommand(const std::string& input)
	{
		const auto tokens = split(input);
		return tokens;

		std::cout << "tokens: " << std::endl;
		for( const auto& token : tokens)
		{
			std::cout << token << std::endl;
		}
	}
};

struct Command
{
	/*
	 * Parametry prikazu
	 */
	std::vector<std::string> params;

	/**
	 * Nazev prikazu
	 */
	std::string commandName;
};
