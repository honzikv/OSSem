#include "ConsoleIn.h"

#include "../Process/ProcessManager.h"
#include "../Utils/Logging.h"


kiv_os::NOS_Error ConsoleIn::Read(char* target_buffer, const size_t buffer_size, size_t& bytes_read) {
	auto regs = kiv_hal::TRegisters();
	auto idx = size_t{0};

	while (idx < buffer_size) {
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

		switch (static_cast<kiv_hal::NControl_Codes>(regs.rax.l)) {
			// NOLINT(clang-diagnostic-switch-enum)
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
			case kiv_hal::NControl_Codes::LF: {
				target_buffer[idx] = symbol;
				idx += 1;
				bytes_read = idx;
				break;
			}
			case kiv_hal::NControl_Codes::NUL: {
				bytes_read = idx; // Zde k indexu 1 nepridavame, protoze CR znak nas nezajima
				return kiv_os::NOS_Error::Success;
			}
			case kiv_hal::NControl_Codes::CR: {
				target_buffer[idx] = symbol;
				idx += 1;
				bytes_read = idx;
				return kiv_os::NOS_Error::Success;
			}
			case kiv_hal::NControl_Codes::EOT:
			case kiv_hal::NControl_Codes::ETX:
			case kiv_hal::NControl_Codes::SUB: {
				target_buffer[idx] = symbol;
				idx += 1;
				bytes_read = idx;
				return kiv_os::NOS_Error::Success; // dostali jsme signal ukoncime cteni
			}
			default: {
				target_buffer[idx] = symbol;
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


bool ConsoleIn::Has_Control_Char(kiv_hal::NControl_Codes control_char) {
	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.l)>(kiv_hal::NKeyboard::Peek_Char);
	regs.rdx.l = static_cast<char>(control_char);
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, regs);

	return regs.flags.non_zero != 0;
}

kiv_os::NOS_Error ConsoleIn::Close() {
	constexpr auto eot_symbol = static_cast<char>(kiv_hal::NControl_Codes::EOT);

	// Zapiseme EOT na vystup, pokud tam uz neni
	// if (Has_Control_Char(kiv_hal::NControl_Codes::EOT)) {
	// 	return kiv_os::NOS_Error::Success;
	// }

	auto regs = kiv_hal::TRegisters();
	regs.rax.h = static_cast<decltype(regs.rax.l)>(kiv_hal::NKeyboard::Write_Char);
	regs.rdx.l = eot_symbol;
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, regs);
	return kiv_os::NOS_Error::Success;
}
