#pragma once
#include <regex>
#include <string>
#include <vector>
#include <locale>

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

	inline std::string trimFromLeft(const std::string& str) {
		return std::regex_replace(str, std::regex("^\\s+"), std::string(""));
	}

	inline std::string trimFromRight(const std::string& str) {
		return std::regex_replace(str, std::regex("\\s+$"), std::string(""));
	}

	/**
	 * Odstrani mezery z retezce
	 */
	inline std::string trimWhitespaces(const std::string& str) {
		return trimFromLeft(trimFromRight(str));
	}
	

}
