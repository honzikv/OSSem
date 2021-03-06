#pragma once
#include <memory>
#include <sstream>

namespace ByteUtils {

	template <typename T>
	inline void Convert_To_Char_Array(T& item, char* array) {
		std::memcpy(array, std::addressof(item) ,sizeof(T));
	}

	template <typename T>
	inline void Copy_To_String_Stream(T item, std::stringstream& string_stream) {
		const auto bytes = sizeof(T);
		char buffer[bytes];
		Convert_To_Char_Array(item, buffer);
		for (size_t i = 0; i < bytes; i += 1) {
			string_stream << buffer[i];
		}
	}
}