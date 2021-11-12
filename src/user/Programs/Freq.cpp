#include "rtl.h"
#include <array>
#include <iomanip>
#include <sstream>
#include <string_view>
#include "Freq.h"

constexpr auto BUFFER_SIZE = 256;
constexpr auto FREQ_TABLE_SIZE = 256; // cteme byty a muzeme mit az 2^8 variant cisel - tzn 256

// EOT se pouzije pro ukonceni vypoctu frekvenci
constexpr auto EOT_SYMBOL = static_cast<char>(kiv_hal::NControl_Codes::EOT);

extern "C" size_t __stdcall freq(const kiv_hal::TRegisters& regs) {
	// Newline do konzole
	const auto newline = std::string("\n");

	// Ziskame vstup pro cteni
	const auto std_in = static_cast<kiv_os::THandle>(regs.rbx.x);

	// Zde ani nepotrebujeme hashmapu / mapu, ale staci pole, protoze je bytu "malo"
	auto byteFrequencies = std::array<int, FREQ_TABLE_SIZE>();

	// Buffer pro data
	auto buffer = std::array<char, BUFFER_SIZE>();

	// Counter pro pocet prectenych/zapsanych bytu v bufferu
	auto bytesFromStream = size_t();

	// Cteme dokud nenarazime na EOT
	auto continue_reading = true;
	while (continue_reading) {
		if (kiv_os_rtl::ReadFile(std_in, buffer.data(), BUFFER_SIZE, bytesFromStream)) {
			for (auto i = 0; i < static_cast<int>(bytesFromStream); i += 1) {
				const auto byte = static_cast<uint8_t>(buffer[i]); // pretypujeme dany symbol na uint8_t byte
				if (byte == EOT_SYMBOL) { // Nasli jsme EOT a ukoncime for loop + while loop
					continue_reading = false;
					break;
				}

				byteFrequencies[byte] += 1; // Zvysime pocet vyskytu v tabulce
			}
		}
		else {
			break;
		}
	}

	const auto std_out = static_cast<kiv_os::THandle>(regs.rax.x);
	auto stringStream = std::stringstream(); // pro formatovani je nejsnazsi pouzit string stream
	for (auto i = uint64_t{ 0 }; i < byteFrequencies.size(); i += 1) {
		const auto byteFreq = byteFrequencies[0];
		if (byteFreq == 0) {
			continue;
		}
		stringStream.str(""); // vycisteni streamu
		// Zformatujeme
		stringStream << "0x" << std::setw(2) << std::hex << i << std::dec << ": " << byteFreq << std::endl;

		auto output = stringStream.str(); // ziskame string

		// Zkusime zapsat output a newline, pokud jedno selze ukoncime program s chybou
		if (kiv_os_rtl::WriteFile(std_out, output.data(), output.size(), bytesFromStream)
			&& kiv_os_rtl::WriteFile(std_out, newline.data(), newline.size(), bytesFromStream)) {
			continue;
		}

		return static_cast<size_t>(kiv_os::NOS_Error::IO_Error);
	}

	return static_cast<size_t>(kiv_os::NOS_Error::Success);
}
