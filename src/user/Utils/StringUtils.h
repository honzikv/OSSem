#pragma once
#include <regex>
#include <string>
#include <vector>
#include <locale>
#include <numeric>
#include <sstream>

/// <summary>
/// Funkce pro praci s retezci
/// </summary>
namespace StringUtils {


	/// <summary>
	/// Rozdeli retezec podle regexu
	/// </summary>
	/// <param name="source">Retezec, ktery se ma rozdelit</param>
	/// <param name="regex">Regex, podle ktereho se retezec rozdeli</param>
	/// <returns></returns>
	inline auto SplitByRegex(const std::string& source, const std::regex& regex) {
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
	inline std::string TrimFromLeft(const std::string& str) {
		return std::regex_replace(str, std::regex("^\\s+"), std::string(""));
	}

	/// <summary>
	/// Orezani mezer zprava
	/// </summary>
	/// <param name="str">Retezec, ktery se orizne</param>
	/// <returns></returns>
	inline std::string TrimFromRight(const std::string& str) {
		return std::regex_replace(str, std::regex("\\s+$"), std::string(""));
	}


	/// <summary>
	/// Odstrani mezery z retezce zleva i zprava
	/// </summary>
	/// <param name="str">Retezec, ze kterho se maji retezce odstranit</param>
	/// <returns></returns>
	inline std::string TrimWhitespaces(const std::string& str) {
		return TrimFromLeft(TrimFromRight(str));
	}

	/// <summary>
	/// Spoji prvky pomoci delimiteru
	/// </summary>
	/// <param name="vector">Vektor, ktery cheme spojit</param>
	/// <param name="delim">Delimiter</param>
	/// <returns></returns>
	inline std::string JoinByDelimiter(const std::vector<std::string>& vector, const std::string& delim) {
		if (vector.size() == 0) {
			return "";
		}
		
		return std::accumulate(
			std::next(vector.begin()),
			vector.end(),
			vector[0],
			[delim](const std::string left, const std::string right) {
				return left + delim + right;
			}
		);
	}


}
