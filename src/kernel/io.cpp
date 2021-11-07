#include "io.h"
#include "kernel.h"
#include "handles.h"
#include <iostream>

#include "IO/AbstractFile.h"

/// <summary>
/// Provede cteni ze souboru - pro soubor zavola metodu read()
///
///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
/// </summary>
/// <param name="regs">Kontext</param>
void performRead(kiv_hal::TRegisters& regs) {
	const auto fileHandle = Resolve_kiv_os_Handle(regs.rdx.x);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		regs.rax.r = -1;
		regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
	}
	
	const auto file = static_cast<AbstractFile*>(fileHandle);
	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r; // kolik bytu se ma precist
	auto bytesRead = size_t{ 0 }; // pocet prectenych bytu
	
	const auto result = file->read(buffer, bytes, bytesRead);
	// Nastavime pocet prectenych bytu a vysledek operace

	// Pokud je vysledek success vratime pocet bytu v rax
	if (result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytesRead;
		return;
	}

	// Jinak nastavime flag a do rax zapiseme chybu
	regs.flags.carry = 1;
	regs.rax.e = static_cast<decltype(regs.rax.e)>(result);
}

/// <summary>
/// Provede zapis do souboru - pro soubor zavola metodu write().
///
///	Funkce vraci File_Not_Found, pokud handle k pozadovanemu souboru neexistuje
/// </summary>
/// <param name="regs">Kontext</param>
void performWrite(kiv_hal::TRegisters& regs) {
	const auto fileHandle = Resolve_kiv_os_Handle(regs.rdx.x);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		regs.rax.r = -1;
		regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
	}

	// File handle je void pointer, ktery muzeme pretypovat na nas objekt
	const auto file = static_cast<AbstractFile*>(fileHandle);
	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r;

	auto bytesWritten = size_t{ 0 };

	const auto result = file->write(buffer, bytes, bytesWritten);
	// Nastavime pocet prectenych bytu a vysledek operace

	// Pokud je vysledek Success vratime pocet bytu v rax
	if (result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytesWritten;
		return;
	}

	// Jinak nastavime flag a do rax zapiseme chybu
	regs.flags.carry = 1;
	regs.rax.e = static_cast<decltype(regs.rax.e)>(result);
}


void Handle_IO(kiv_hal::TRegisters& regs) {
	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {

		case kiv_os::NOS_File_System::Read_File: {
			performRead(regs);
			break;
		}

		case kiv_os::NOS_File_System::Write_File: {
			performWrite(regs);
			break;
		}
		case kiv_os::NOS_File_System::Open_File: {
			
		}
	}
}
