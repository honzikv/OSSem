#include "IOManager.h"

#include "ReadablePipe.h"
#include "WritablePipe.h"
#include "Utils/Logging.h"

void IOManager::HandleIO(kiv_hal::TRegisters& regs) {
	const auto operation = regs.rax.l;
	auto result = kiv_os::NOS_Error::Success;
	switch (static_cast<kiv_os::NOS_File_System>(operation)) {
		case kiv_os::NOS_File_System::Read_File: {
			result = PerformRead(regs);
			break;
		}

		case kiv_os::NOS_File_System::Write_File: {
			result = PerformWrite(regs);
			break;
		}

		case kiv_os::NOS_File_System::Create_Pipe: {
			result = CreatePipe(regs);
			break;
		}

		case kiv_os::NOS_File_System::Close_Handle: {
			result = CloseHandle(regs);
			break;
		}

		case kiv_os::NOS_File_System::Set_Working_Dir: {
			result = SetWorkingDir(regs);
			break;
		}

		case kiv_os::NOS_File_System::Get_Working_Dir: {
			result = GetWorkingDir(regs);
			break;
		}

		case kiv_os::NOS_File_System::Get_File_Attribute: {
			result = GetFileAttribute(regs);
			break;
		}

		case kiv_os::NOS_File_System::Open_File: {
			result = OpenFsFile(regs);
			break;
		}

	}

}

auto IOManager::CreateStdIO() -> std::pair<kiv_os::THandle, kiv_os::THandle> {
	const auto std_in = std::make_shared<ConsoleIn>();
	const auto std_out = std::make_shared<ConsoleOut>();

	const auto std_in_handle = HandleService::Get().CreateEmptyHandle();
	const auto std_out_handle = HandleService::Get().CreateEmptyHandle();

	open_files[std_in_handle] = std_in;
	open_files[std_out_handle] = std_out;

	return {std_in_handle, std_out_handle};
}

auto IOManager::MapProcessStdio(const kiv_os::THandle std_in,
                                const kiv_os::THandle std_out) -> std::pair<kiv_os::THandle, kiv_os::THandle> {
	// Ziskame nove handles
	const auto std_in_new_handle = HandleService::Get().CreateEmptyHandle();
	const auto std_out_new_handle = HandleService::Get().CreateEmptyHandle();

	// Zamkneme
	auto lock = std::scoped_lock(mutex);

	// Pokud nektery soubor neexistuje nebo jsou nove handly invalid (dosli), vratime chybu
	if (open_files.count(std_in) == 0 || open_files.count(std_out) == 0
		|| std_in_new_handle == kiv_os::Invalid_Handle || std_out_new_handle == kiv_os::Invalid_Handle) {
		return {kiv_os::Invalid_Handle, kiv_os::Invalid_Handle};
	}

	// Jinak nastavime stdin a stdout
	open_files[std_in_new_handle] = open_files.at(std_in);
	open_files[std_out_new_handle] = open_files.at(std_out);

	return {std_in_new_handle, std_out_new_handle};
}

void IOManager::CloseProcessStdio(const kiv_os::THandle std_in,
                                  const kiv_os::THandle std_out) {
	auto erase_std_in = false;
	auto erase_std_out = false;
	{
		auto lock = std::scoped_lock(mutex);

		if (open_files.count(std_in) > 1) {
			open_files[std_in]->Close();
			open_files.erase(std_in);
			erase_std_in = true;
		}

		if (open_files.count(std_out) > 1) {
			open_files[std_out]->Close();
			open_files.erase(std_out);
			erase_std_out = true;
		}
	}

	// Pokud se smazalo std_in uvolnime handle
	if (erase_std_in) {
		HandleService::Get().RemoveHandle(std_in);
	}

	// Pokud se smazalo std_out uvolnime handle
	if (erase_std_out) {
		HandleService::Get().RemoveHandle(std_out);
	}

}


kiv_os::NOS_Error IOManager::PerformRead(kiv_hal::TRegisters& regs) {
	std::shared_ptr<IFile> file;
	{
		const auto file_handle = regs.rdx.x;
		auto lock = std::scoped_lock(mutex);
		if (open_files.count(file_handle) == 0) {
			regs.rax.r = -1;
			regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
			return kiv_os::NOS_Error::File_Not_Found;
		}
		file = open_files.at(file_handle);
	}

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
	std::shared_ptr<IFile> file;
	const auto file_handle = regs.rdx.x;
	{
		auto lock = std::scoped_lock(mutex);
		if (open_files.count(file_handle) == 0) {
			regs.rax.r = -1;
			regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
			return kiv_os::NOS_Error::File_Not_Found;
		}
		file = open_files.at(file_handle);
	}

	const auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto bytes = regs.rcx.r;

	auto bytes_written = size_t{0};

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
	auto pipe_write = pipe_read_write_pair[0];
	auto input_preinitialized = false;
	auto pipe_read = pipe_read_write_pair[1];

	// Pipe musime vytvorit jako shared pointer, jinak dojde ke kopirovani a my potrebujeme
	// pouze jedno misto v pameti, kam se bude zapisovat
	const auto pipe = std::make_shared<Pipe>();

	// Aby se zavolali spravne metody pro zavreni, musi byt pipe ve svem danem wrapperu, ktery zavola
	// bud close pro cteni nebo pro zapis
	const auto readable_pipe = std::make_shared<ReadablePipe>(pipe);
	const auto writable_pipe = std::make_shared<WritablePipe>(pipe);

	// Pokud je zapis do pipy invalid, musime ziskat novy handle
	if (pipe_write == kiv_os::Invalid_Handle) {
		input_preinitialized = true;
		const auto file_descriptor = HandleService::Get().CreateEmptyHandle();
		if (file_descriptor == kiv_os::Invalid_Handle) {
			// dosly handly, vyhodime vyjimku
			return kiv_os::NOS_Error::Out_Of_Memory;
		}
		// Jinak nastavime file descriptor
		pipe_write = file_descriptor;
	}

	// To same pro cteni z pipy
	if (pipe_read == kiv_os::Invalid_Handle) {
		const auto file_descriptor = HandleService::Get().CreateEmptyHandle();
		if (file_descriptor == kiv_os::Invalid_Handle) {
			// dosly handly, vyhodime vyjimku
			if (input_preinitialized) {
				// Navic musime jeste provest check, zda-li je potreba odstranit i input handle
				HandleService::Get().RemoveHandle(pipe_write);
			}

			return kiv_os::NOS_Error::Out_Of_Memory;
		}

		pipe_read = file_descriptor;
	}

	// Pridame pipe do tabulky otevrenych souboru
	{
		auto lock = std::scoped_lock(mutex);
		open_files[pipe_write] = std::dynamic_pointer_cast<IFile>(writable_pipe);
		open_files[pipe_read] = std::static_pointer_cast<IFile>(readable_pipe);
		LogDebug("Input fd: " + std::to_string(pipe_write) + " Output fd: " + std::to_string(pipe_read));
	}

	// Zapiseme vysledky zpet do pole
	pipe_read_write_pair[0] = pipe_write;
	pipe_read_write_pair[1] = pipe_read;
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::CloseHandle(const kiv_hal::TRegisters& regs) {
	const auto handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	auto lock = std::scoped_lock(mutex);
	const auto file = open_files[handle];

	if (file == nullptr) {
		return kiv_os::NOS_Error::Success;
	}

	file->Close();
	open_files.erase(handle);

	return kiv_os::NOS_Error::Success;
}
