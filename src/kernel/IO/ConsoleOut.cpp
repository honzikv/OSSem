#include "ConsoleOut.h"
kiv_os::NOS_Error ConsoleOut::write(const char* sourceBuffer, size_t bufferSize, size_t& bytesWritten) {
	auto regs = kiv_hal::TRegisters();

	// Zadame cislo sluzby pro zapis do konzole do AH (RAX -> H)
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);

	// Zadame co se ma zapsat
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(sourceBuffer);

	// Pocet znaku co se ma zapsat
	regs.rcx.r = bufferSize;

	// Zavolame stdout
	Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);

	// Kontrola jestli jsme zapsali vsechny znaky, pokud ne doslo k chybe zapisu
	if (!(regs.rax.r == 0)) {
		// Doslo k chybe
		bytesWritten = UINT32_MAX;

		// Vratime IO error a nastavime pocet zapsanych bytu na uint32_t{-1}
		return kiv_os::NOS_Error::IO_Error;
	}

	// Jinak vratime pocet zapsanych bytu (Pro VGA bios bereme ze se zapsaly byty vsechny)
	bytesWritten = bufferSize;
	return kiv_os::NOS_Error::Success;
}
