#include "ConsoleOut.h"
kiv_os::NOS_Error ConsoleOut::Write(const char* source_buffer, size_t bytes, size_t& bytes_written) {
	auto regs = kiv_hal::TRegisters();

	// Zadame cislo sluzby pro zapis do konzole do AH (RAX -> H)
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);

	// Zadame co se ma zapsat
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(source_buffer);

	// Pocet znaku co se ma zapsat
	regs.rcx.r = bytes;

	// Zavolame stdout
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);

	// Kontrola jestli jsme zapsali vsechny znaky, pokud ne doslo k chybe zapisu
 	if (regs.rax.r == 1) {
		// Doslo k chybe
		bytes_written = -1;

		// Vratime IO error a nastavime pocet zapsanych bytu na uint32_t{-1}
		return kiv_os::NOS_Error::IO_Error;
	}

	// Jinak vratime pocet zapsanych bytu (Pro VGA bios bereme ze se zapsaly byty vsechny)
	bytes_written = bytes;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ConsoleOut::Close() {
	return kiv_os::NOS_Error::Success;
}
