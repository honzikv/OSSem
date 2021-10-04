#pragma once
#include <regex>
#include <string>
#include <vector>
#include <sstream>

/**
 * Funkce pro praci s retezci
 */
namespace StringUtils {
	
	/**
	 * Rozdeli retezec "source" podle regexu "regex"
	 */
	inline auto splitByRegex(const std::string& source, const std::regex& regex) {
		return std::vector<std::string>(
			std::sregex_token_iterator(source.begin(), source.end(), regex, -1),
			std::sregex_token_iterator()
		);
	}
}
