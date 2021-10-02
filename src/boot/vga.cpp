#include "vga.h"

#include <Windows.h>

HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);


void Write_Control_Char(const char ch) {
	//Narozdil od Write_String tady pouzijeme WriteConsole, protoze WriteConsole se pri zapisu nepresmeruje do souboru jako WriteFile.
	//Coz je chovani, ktere u zapisu kontrolniho znaku chceme.

	DWORD read;
	switch (static_cast<kiv_hal::NControl_Codes>(ch)) {
		case kiv_hal::NControl_Codes::BS:
				{
					char backspace_sequence[3] = { 8, ' ', 8 };	//posuneme kurzor o jednu pozici zpet, prepiseme zobrazeny znak, a opet posunme po zapisu kurzor o jednu pozici zpet
					WriteConsoleA(hConsoleOutput, &backspace_sequence, sizeof(backspace_sequence), &read, NULL);
				}
				break;

		default: 
			//kod, ktery nezname proste zapiseme jak je
			WriteConsoleA(hConsoleOutput, &ch, 1, &read, NULL);
			break;
	}
}

void Write_String(char *chars, const size_t count) {
	DWORD read;

	WriteFile(hConsoleOutput, chars, static_cast<DWORD>(count), &read, NULL);
		//nelze pouzit WriteConsoleA, viz komentar u Write_Control_Char
}


void __stdcall VGA_Handler(kiv_hal::TRegisters &context) {
	switch (static_cast<kiv_hal::NVGA_BIOS>(context.rax.h)) {
		case kiv_hal::NVGA_BIOS::Write_Control_Char:		
			Write_Control_Char(context.rdx.l);
			break;
											
		case kiv_hal::NVGA_BIOS::Write_String:	
			Write_String(reinterpret_cast<char*>(context.rdx.r), static_cast<size_t>(context.rcx.r));
			break;

		default: context.flags.carry = 1;
	}
}