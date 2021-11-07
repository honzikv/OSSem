#pragma once
#include <regex>
#include <string>
#include <vector>
#include <locale>

/**
 * Funkce pro praci s retezci
 */
namespace StringUtils {


	/// <summary>
	/// Rozdeli retezec podle regexu
	/// </summary>
	/// <param name="source">Retezec, ktery se ma rozdelit</param>
	/// <param name="regex">Regex, podle ktereho se retezec rozdeli</param>
	/// <returns></returns>
	inline auto splitByRegex(const std::string& source, const std::regex& regex) {
		return std::vector<std::string>(
			std::sregex_token_iterator(source.begin(), source.end(), regex, -1),
			std::sregex_token_iterator()
		);
	}

	/// <summary>
	/// Orezani mezer zleva
	/// </summary>
	/// <param name="str">Retezec, ktery se orizne</param>
	/// <returns></returns>
	inline std::string trimFromLeft(const std::string& str) {
		return std::regex_replace(str, std::regex("^\\s+"), std::string(""));
	}

	/// <summary>
	/// Orezani mezer zprava
	/// </summary>
	/// <param name="str">Retezec, ktery se orizne</param>
	/// <returns></returns>
	inline std::string trimFromRight(const std::string& str) {
		return std::regex_replace(str, std::regex("\\s+$"), std::string(""));
	}
	

	/// <summary>
	/// Odstrani mezery z retezce zleva i zprava
	/// </summary>
	/// <param name="str">Retezec, ze kterho se maji retezce odstranit</param>
	/// <returns></returns>
	inline std::string trimWhitespaces(const std::string& str) {
		return trimFromLeft(trimFromRight(str));
	}
	

}
