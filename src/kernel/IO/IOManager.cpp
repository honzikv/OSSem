#include "IOManager.h"

auto IOManager::CreateStdIO() -> std::pair<kiv_os::THandle, kiv_os::THandle> {
	const auto std_in = std::make_shared<ConsoleIn>();
	const auto std_out = std::make_shared<ConsoleOut>();

	const auto std_in_handle = HandleService::Get().CreateHandle(std_in.get());
	const auto std_out_handle = HandleService::Get().CreateHandle(std_out.get());

	open_files[std_in_handle] = std_in;
	open_files[std_out_handle] = std_out;

	return { std_in_handle, std_out_handle };
}

kiv_os::NOS_Error IOManager::PerformRead(kiv_hal::TRegisters& regs) {
	const auto fileHandle = HandleService::Get().GetHandle(regs.rdx.x);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		regs.rax.r = -1;
		regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
	}

	const auto file = static_cast<IFile*>(fileHandle);
	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r; // kolik bytu se ma precist
	auto bytes_read = size_t{0}; // pocet prectenych bytu

	const auto result = file->Read(buffer, bytes, bytes_read);

	// Pokud je vysledek success vratime pocet bytu v rax
	if (result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytes_read;
		return kiv_os::NOS_Error::Success;
	}

	// Jinak nastavime flag a do rax zapiseme chybu
	regs.flags.carry = 1;
	regs.rax.e = static_cast<decltype(regs.rax.e)>(result);
	return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error IOManager::PerformWrite(kiv_hal::TRegisters& regs) {
	const auto file_handle = HandleService::Get().GetHandle(regs.rdx.x);
	if (file_handle == INVALID_HANDLE_VALUE) {
		regs.rax.r = -1;
		regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
	}

	// File handle je void pointer, ktery muzeme pretypovat na nas objekt
	const auto file = static_cast<IFile*>(file_handle);
	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r;

	auto bytes_written = size_t{ 0 };

	const auto result = file->Write(buffer, bytes, bytes_written);

	// Pokud je vysledek Success vratime pocet bytu v rax
	if (result == kiv_os::NOS_Error::Success) {
		regs.rax.r = bytes_written;
		return kiv_os::NOS_Error::Success;
	}

	// Jinak nastavime flag a do rax zapiseme chybu
	regs.flags.carry = 1;
	regs.rax.e = static_cast<decltype(regs.rax.e)>(result);
	return kiv_os::NOS_Error::IO_Error;
}

kiv_os::NOS_Error IOManager::CreatePipe(const kiv_hal::TRegisters& regs) {
	const auto pipe_read_write_pair = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	auto input = pipe_read_write_pair[0];
	auto input_preinitialized = false;
	auto output = pipe_read_write_pair[1];

	// Pipe musime vytvorit jako shared pointer, jinak dojde ke kopirovani a my potrebujeme
	// pouze jedno misto v pameti, kam se bude zapisovat
	const auto pipe = std::make_shared<Pipe>();

	// Pokud je vstup invalid value musime pro nej vytvorit novy file descriptor
	if (input == kiv_os::Invalid_Handle) {
		input_preinitialized = true;
		const auto file_descriptor = HandleService::Get().CreateHandle(pipe.get());
		if (file_descriptor == kiv_os::Invalid_Handle) {
			// dosly handly, vyhodime vyjimku
			return kiv_os::NOS_Error::Out_Of_Memory;
		}
		// Jinak nastavime file descriptor
		input = file_descriptor;
	}

	// To same pro vystup
	if (output == kiv_os::Invalid_Handle) {
		const auto file_descriptor = HandleService::Get().CreateHandle(pipe.get());
		if (file_descriptor == kiv_os::Invalid_Handle) {
			// dosly handly, vyhodime vyjimku
			if (input_preinitialized) {
				// Navic musime jeste provest check, zda-li je potreba odstranit i input handle
				HandleService::Get().RemoveHandle(input);
			}

			return kiv_os::NOS_Error::Out_Of_Memory;
		}

		output = file_descriptor;
	}

	// Pridame pipe do tabulky otevrenych souboru
	open_files[input] = std::static_pointer_cast<IFile>(pipe);
	open_files[output] = std::static_pointer_cast<IFile>(pipe);

	// Zapiseme vysledky zpet do pole
	pipe_read_write_pair[0] = input;
	pipe_read_write_pair[1] = output;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::CloseHandle(const kiv_hal::TRegisters& regs) {
	if (const auto handle = static_cast<kiv_os::THandle>(regs.rdx.x); open_files.count(handle) > 0) {
		open_files.erase(handle);
	}
	return kiv_os::NOS_Error::Success;
}
