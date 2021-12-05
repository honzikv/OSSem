#pragma once
#include <regex>
#include <string>
#include <vector>
#include <locale>
#include <numeric>
#include "../../api/hal.h"
#include "../../api/api.h"
#include <sstream>

/// <summary>
/// Funkce pro praci s retezci
/// </summary>
namespace StringUtils {

	const std::regex whitespace_regex = std::regex("\\s+");

	/// <summary>
	/// Rozdeli retezec podle regexu
	/// </summary>
	/// <param name="source">Retezec, ktery se ma rozdelit</param>
	/// <param name="regex">Regex, podle ktereho se retezec rozdeli</param>
	/// <returns></returns>
	inline auto Split_By_Regex(const std::string& source, const std::regex& regex) {
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
	inline std::string Trim_From_Left(const std::string& str) {
		return std::regex_replace(str, std::regex("^\\s+"), std::string(""));
	}

	/// <summary>
	/// Orezani mezer zprava
	/// </summary>
	/// <param name="str">Retezec, ktery se orizne</param>
	/// <returns></returns>
	inline std::string Trim_From_Right(const std::string& str) {
		return std::regex_replace(str, std::regex("\\s+$"), std::string(""));
	}


	/// <summary>
	/// Odstrani mezery z retezce zleva i zprava
	/// </summary>
	/// <param name="str">Retezec, ze kterho se maji retezce odstranit</param>
	/// <returns></returns>
	inline std::string Trim_Whitespaces(const std::string& str) { return Trim_From_Left(Trim_From_Right(str)); }

	/// <summary>
	/// Spoji prvky pomoci delimiteru
	/// </summary>
	/// <param name="vector">Vektor, ktery cheme spojit</param>
	/// <param name="delim">Delimiter</param>
	/// <returns></returns>
	inline std::string Join_By_Delimiter(const std::vector<std::string>& vector, const std::string& delim) {
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

	/// <summary>
	/// Vrati, zda-li je symbol CTRL D
	/// </summary>
	/// <param name="symbol"></param>
	/// <returns></returns>
	inline bool Is_Ctrl_D(const char symbol) { return symbol == static_cast<char>(kiv_hal::NControl_Codes::EOT); }

	/// <summary>
	/// Vrati zda-li je symbol CTRL Z
	/// </summary>
	/// <param name="symbol"></param>
	/// <returns></returns>
	inline bool Is_Ctrl_Z(const char symbol) { return symbol == static_cast<char>(kiv_hal::NControl_Codes::SUB); }

	/// <summary>
	/// Vrati zda-li je symbol CTRL C
	/// </summary>
	/// <param name="symbol"></param>
	/// <returns></returns>
	inline bool Is_Ctrl_C(const char symbol) { return symbol == static_cast<char>(kiv_hal::NControl_Codes::ETX); }


	inline bool Is_CR(const char symbol) { return symbol == static_cast<char>(kiv_hal::NControl_Codes::CR); }


	inline std::vector<std::string> FilterEmptyStrings(const std::vector<std::string>& vector) {
		auto result = std::vector<std::string>();

		for (const auto token : vector) {
			if (!std::regex_match(token.begin(), token.end(), whitespace_regex) && token.size() > 0) {
				result.push_back(Trim_Whitespaces(token));
			}
		}

		return result;
	}

	inline std::string Err_To_String(const kiv_os::NOS_Error err) {
		switch (err) {
			case kiv_os::NOS_Error::Directory_Not_Empty: return "DirectoryNotEmpty";
			case kiv_os::NOS_Error::File_Not_Found: return "FileNotFound";
			case kiv_os::NOS_Error::IO_Error: return "IOError";
			case kiv_os::NOS_Error::Invalid_Argument: return "InvalidArgument";
			case kiv_os::NOS_Error::Not_Enough_Disk_Space: return "NotEnoughDiskSpace";
			case kiv_os::NOS_Error::Out_Of_Memory: return "OutOfMemory";
			case kiv_os::NOS_Error::Permission_Denied: return "PermissionDenied";
			case kiv_os::NOS_Error::Success: return "Success";
			case kiv_os::NOS_Error::Unknown_Error: return "UnknownError";
			case kiv_os::NOS_Error::Unknown_Filesystem: return "UnknownFilesystem";
			default: return "InvalidEnumValue";
		}
	}
}
