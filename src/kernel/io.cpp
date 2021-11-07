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
void Perform_Read(kiv_hal::TRegisters& regs) {
	const auto fileHandle = Resolve_kiv_os_Handle(regs.rdx.x);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		regs.rax.r = -1;
		regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
	}
	
	const auto file = static_cast<AbstractFile*>(fileHandle);
	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r; // kolik bytu se ma precist
	auto bytes_read = size_t{ 0 }; // pocet prectenych bytu
	
	const auto result = file->Read(buffer, bytes, bytes_read);

	// Pokud je vysledek success vratime pocet bytu v rax
	if (result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytes_read;
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
void Perform_Write(kiv_hal::TRegisters& regs) {
	const auto file_handle = Resolve_kiv_os_Handle(regs.rdx.x);
	if (file_handle == INVALID_HANDLE_VALUE) {
		regs.rax.r = -1;
		regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
	}

	// File handle je void pointer, ktery muzeme pretypovat na nas objekt
	const auto file = static_cast<AbstractFile*>(file_handle);
	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r;

	auto bytes_written = size_t{ 0 };

	const auto result = file->Write(buffer, bytes, bytes_written);

	// Pokud je vysledek Success vratime pocet bytu v rax
	if (result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytes_written;
		return;
	}

	// Jinak nastavime flag a do rax zapiseme chybu
	regs.flags.carry = 1;
	regs.rax.e = static_cast<decltype(regs.rax.e)>(result);
}


void Handle_IO(kiv_hal::TRegisters& regs) {
	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {

		case kiv_os::NOS_File_System::Read_File: {
			Perform_Read(regs);
			break;
		}

		case kiv_os::NOS_File_System::Write_File: {
			Perform_Write(regs);
			break;
		}
		case kiv_os::NOS_File_System::Open_File: {
			
		}
	}
}
