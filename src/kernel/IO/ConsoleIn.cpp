#include "ConsoleIn.h"
kiv_os::NOS_Error ConsoleIn::read(char* targetBuffer, const size_t bytes, size_t& bytesRead) {
	auto regs = kiv_hal::TRegisters();
	auto idx = uint32_t{0};
	while (idx < bytes) {
		// Precteme znak
		regs.rax.h = static_cast<decltype(regs.rax.l)>(kiv_hal::NKeyboard::Read_Char);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, regs);

		// Pokud je nastaveny flag vratime chybu
		if (!regs.flags.non_zero) {
			return kiv_os::NOS_Error::IO_Error;
		}

		// Jinak kontrolujeme co je to za znak - pokud se jedna o specialni symbol osetrime a pokud ne pridame ho do
		// bufferu
		auto symbol = static_cast<char>(regs.rax.l);

		switch (static_cast<kiv_hal::NControl_Codes>(regs.rax.l)) {  // NOLINT(clang-diagnostic-switch-enum)
			case kiv_hal::NControl_Codes::BS: {
				// Backspace = smazeme znak z bufferu

				// Pokud je index vetsi nez 1 snizime
				idx = idx > 0 ? idx - 1 : idx;

				// Zapiseme znak do VGA biosu - tzn smazeme z konzole znak
				regs.rax.h = static_cast<decltype(regs.rax.l)>(kiv_hal::NVGA_BIOS::Write_Control_Char);
				regs.rdx.l = symbol;
				kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);
				break;
			}
				// Pro LF nepridavame ani index a jenom pokracujeme ve while loopu
			case kiv_hal::NControl_Codes::LF: {
				break;
			}
			case kiv_hal::NControl_Codes::NUL:
			case kiv_hal::NControl_Codes::CR: {
				bytesRead = idx; // Zde k indexu 1 nepridavame, protoze CR znak nas nezajima
				return kiv_os::NOS_Error::Success;
			}

			default: {
				targetBuffer[idx] = symbol;
				idx += 1;

				regs.rax.h = static_cast<decltype(regs.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
				regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&symbol);
				regs.rcx.r = 1;
				kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);
				break;
			}
		}
	}

	// Pokud jsme dobehli az sem tak vratime success
	return kiv_os::NOS_Error::Success;
}
