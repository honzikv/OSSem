#include "IOManager.h"

#include "ReadablePipe.h"
#include "WritablePipe.h"
#include "Utils/Logging.h"

void IOManager::HandleIO(kiv_hal::TRegisters &regs) {
    const auto operation = regs.rax.l;
    auto operation_result = kiv_os::NOS_Error::Success;
    switch (static_cast<kiv_os::NOS_File_System>(operation)) {
        case kiv_os::NOS_File_System::Read_File: {
            operation_result = PerformRead(regs);
            break;
        }

        case kiv_os::NOS_File_System::Write_File: {
            operation_result = PerformWrite(regs);
            break;
        }

        case kiv_os::NOS_File_System::Create_Pipe: {
            operation_result = PerformCreatePipe(regs);
            break;
        }

        case kiv_os::NOS_File_System::Close_Handle: {
            operation_result = PerformCloseHandle(regs);
            break;
        }

        case kiv_os::NOS_File_System::Set_Working_Dir: {
            operation_result = PerformSetWorkingDir(regs);
            break;
        }

        case kiv_os::NOS_File_System::Get_Working_Dir: {
            operation_result = PerformGetWorkingDir(regs);
            break;
        }

        case kiv_os::NOS_File_System::Get_File_Attribute: {
            operation_result = PerformGetFileAttribute(regs);
            break;
        }

        case kiv_os::NOS_File_System::Open_File: {
            operation_result = PerformOpenFile(regs);
            break;
        }
        case kiv_os::NOS_File_System::Delete_File: {
            operation_result = PerformDeleteFile(regs);
            break;
        }
        case kiv_os::NOS_File_System::Seek: {
            operation_result = PerformSeek(regs);
            break;
        }
        case kiv_os::NOS_File_System::Set_File_Attribute: {
            operation_result = PerformSetFileAttribute(regs);
            break;
        }

    }

    if (operation_result != kiv_os::NOS_Error::Success) {
        regs.flags.carry = 1;
        regs.rax.x = static_cast<decltype(regs.rax.x)>(operation_result);
    }

}

auto IOManager::CreateStdIO() -> std::pair<kiv_os::THandle, kiv_os::THandle> {
    const auto std_in = std::make_shared<ConsoleIn>();
    const auto std_out = std::make_shared<ConsoleOut>();

    const auto std_in_handle = HandleService::Get().CreateEmptyHandle();
    const auto std_out_handle = HandleService::Get().CreateEmptyHandle();

    open_files[std_in_handle] = {1, std_in};
    open_files[std_out_handle] = {1, std_out};

    return {std_in_handle, std_out_handle};
}


kiv_os::NOS_Error IOManager::RegisterProcessStdIO(const kiv_os::THandle std_in, const kiv_os::THandle std_out) {
    auto lock = std::scoped_lock(mutex);

    // Pokud by se nahodou stalo, ze stdin a stdout handly neexistuji vyhodime chybu
    if (open_files.count(std_in) == 0 || open_files.count(std_out) == 0) {
        return kiv_os::NOS_Error::IO_Error;
    }

    // Jinak je ziskame a zvysime jejich vyskyt
    auto[std_in_count, std_in_file_ptr] = open_files[std_in];
    open_files[std_in] = {std_in_count + 1, std_in_file_ptr};
    auto[std_out_count, std_out_file_ptr] = open_files[std_out];
    open_files[std_out] = {std_out_count + 1, std_out_file_ptr};
    return kiv_os::NOS_Error::Success;
}


void IOManager::DecrementFileReference(const kiv_os::THandle handle, bool &handle_removed) {
    if (open_files.count(handle) != 0) {
        auto[count, file] = open_files[handle];
        count -= 1;
        if (count <= 0) {
            open_files.erase(handle);
            file->Close();
            handle_removed = true;
        } else {
            open_files[handle] = {count, file};
        }
    }

}

kiv_os::NOS_Error IOManager::UnregisterProcessStdIO(const kiv_os::THandle std_in, const kiv_os::THandle std_out) {
    auto erase_std_in_handle = false;
    auto erase_std_out_handle = false;
    {
        auto lock = std::scoped_lock(mutex);
        DecrementFileReference(std_in, erase_std_in_handle);
        DecrementFileReference(std_out, erase_std_out_handle);
    }

    // Pokud se smazalo std_in uvolnime handle
    if (erase_std_in_handle) {
        HandleService::Get().RemoveHandle(std_in);
    }

    // Pokud se smazalo std_out uvolnime handle
    if (erase_std_out_handle) {
        HandleService::Get().RemoveHandle(std_out);
    }

    return kiv_os::NOS_Error::Success;
}


kiv_os::NOS_Error IOManager::PerformRead(kiv_hal::TRegisters &regs) {
    std::shared_ptr<IFile> file;
    {
        const auto file_handle = regs.rdx.x;
        auto lock = std::scoped_lock(mutex);
        if (open_files.count(file_handle) == 0) {
            regs.rax.r = -1;
            regs.rbx.e = static_cast<decltype(regs.rbx.e)>(kiv_os::NOS_Error::File_Not_Found);
            return kiv_os::NOS_Error::File_Not_Found;
        }
        file = open_files.at(file_handle).second; // file pointer je druhy prvek v dvojici
    }

    const auto buffer = reinterpret_cast<char *>(regs.rdi.r);
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

kiv_os::NOS_Error IOManager::PerformWrite(kiv_hal::TRegisters &regs) {
    std::shared_ptr<IFile> file;
    {
        const auto file_handle = regs.rdx.x;
        auto lock = std::scoped_lock(mutex);
        if (open_files.count(file_handle) == 0) {
            return kiv_os::NOS_Error::File_Not_Found;
        }
        file = open_files.at(file_handle).second; // file pointer je druhy prvek v dvojici
    }

    const auto buffer = reinterpret_cast<char *>(regs.rdi.r);
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

kiv_os::NOS_Error IOManager::PerformCreatePipe(const kiv_hal::TRegisters &regs) {
    const auto pipe_read_write_pair = reinterpret_cast<kiv_os::THandle *>(regs.rdx.r);
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
    // Zde musime nastavit pocatecni pocet referenci na 0, protoze pipy se nastavuji pri
    // vytvoreni procesu a pokud by byli defaultne 1 pak by se spravne neodstranili
    {
        auto lock = std::scoped_lock(mutex);
        open_files[pipe_write] = {0, std::dynamic_pointer_cast<IFile>(writable_pipe)};
        open_files[pipe_read] = {0, std::static_pointer_cast<IFile>(readable_pipe)};
        LogDebug("Input fd: " + std::to_string(pipe_write) + " Output fd: " + std::to_string(pipe_read));
    }

    // Zapiseme vysledky zpet do pole
    pipe_read_write_pair[0] = pipe_write;
    pipe_read_write_pair[1] = pipe_read;
    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error IOManager::PerformCloseHandle(const kiv_hal::TRegisters &regs) {
    const auto handle = static_cast<kiv_os::THandle>(regs.rdx.x);
    auto lock = std::scoped_lock(mutex);

    if (open_files.count(handle) == 0) {
        // Pokud file neni vratime se
        return kiv_os::NOS_Error::Success;
    }

    auto[count, file] = open_files[handle];

    // Snizime count o 1
    count -= 1;

    // V pripade pipy, ktera se nenastavila procesu muze by inicialni hodnota countu 0, takze po snizeni muze byt i -1
    // tim padem musime udelat check na <= 0
    if (count <= 0) {
        open_files.erase(handle);
        file->Close();

        // Jeste zavolame handle service aby si odstranilo handle
        HandleService::Get().RemoveHandle(handle);
    }

    return kiv_os::NOS_Error::Success;
}

void IOManager::Init_Filesystems() {
    //TODO procs asi
    for (int i = 0; i < 256; ++i) {
        auto disk_num = static_cast<uint8_t>(i);
        kiv_hal::TRegisters registers{};
        kiv_hal::TDrive_Parameters parameters{};
        registers.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);
        registers.rdi.r = reinterpret_cast<uint64_t>(&parameters);
        registers.rdx.l = disk_num;
        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers);
        if (!registers.flags.carry) { // je disk
            auto fat_fs = new Fat12();
            file_systems["C:\\"] = std::unique_ptr<VFS>(fat_fs);
            break;
        }
    }
}

/**
 * Provede operaci seek
 * @param regs registry
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error IOManager::PerformSeek(kiv_hal::TRegisters &regs) {
    std::shared_ptr<IFile> file;
    {
        const auto file_handle = regs.rdx.x;
        auto lock = std::scoped_lock(mutex);
        if (open_files.count(file_handle) == 0) {
            return kiv_os::NOS_Error::File_Not_Found;
        }
        file = open_files.at(file_handle).second; // file pointer je druhy prvek v dvojici
    }
    kiv_os::THandle handle = regs.rdx.x; // handle
    auto new_pos = static_cast<size_t>(regs.rdi.r); // nova pozice
    auto pos_type = static_cast<kiv_os::NFile_Seek>(regs.rcx.l); // typ pozice
    auto operation = static_cast<kiv_os::NFile_Seek>(regs.rcx.h); // operace - (get/set position / set size)
    size_t file_pos;
    if (operation == kiv_os::NFile_Seek::Set_Size) {
        auto res = file->Seek(new_pos, pos_type, operation, file_pos);
    }
    auto res = file->Seek(new_pos, pos_type, operation, file_pos);
    if (res == kiv_os::NOS_Error::Success && operation == kiv_os::NFile_Seek::Set_Position) {
        regs.rax.r = file_pos;
    }
    return res;
}

kiv_os::NOS_Error PerformSetWorkingDir(const kiv_hal::TRegisters& regs) {

}