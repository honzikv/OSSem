//
// Created by Kuba on 18.11.2021.
//

#include "fs_file.h"

Fs_File::Fs_File(VFS *fs, File file) : fs(fs), file(file) {

}

/**
 * Zjisti, jestli doubor dany atribut
 * @param attr atribut
 * @return true pokud soubor ma atribut, jinak false
 */
bool Fs_File::Has_Attr(kiv_os::NFile_Attributes attr) {
    return (static_cast<uint8_t>(file.attributes) & static_cast<uint8_t>(attr)) != 0;
}

/**
 * Zjisti, jestli je dany soubor jen pro cteni
 * @return true, pokud je jen pro cteni, jinak false
 */
bool Fs_File::Is_Read_Only() {
    return Has_Attr(kiv_os::NFile_Attributes::Read_Only);
}

/**
 * Zjisti, jestli je dany soubor adresar
 * @return true, pokud je adresar, jinak false
 */
bool Fs_File::Is_Dir() {
    return Has_Attr(kiv_os::NFile_Attributes::Directory);
}

/**
 * Provede zapis dat do souboru
 * @param source_buffer data, ktera se budou zapisovat
 * @param buffer_size velikost zapisovanych dat
 * @param bytes_written kolik dat bylo skutecne zapsano
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fs_File::Write(const char *source_buffer, size_t buffer_size, size_t &bytes_written) {
    if (Is_Dir() || Is_Read_Only()) {
        return kiv_os::NOS_Error::Permission_Denied;
    }

    std::vector<char> buffer(source_buffer, source_buffer + buffer_size);
    auto res = fs->Write(file, file.offset, buffer, bytes_written);
    file.offset += bytes_written;
    return res;
}

/**
 * Precte data ze souboru do bufferu
 * @param target_buffer Buffer, do ktereho se data zapisuji
 * @param buffer_size Pocet bytu, ktery se ma precist
 * @param bytes_read Pocet prectenych bytu
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fs_File::Read(char *target_buffer, size_t buffer_size, size_t &bytes_read) {
    buffer_size = std::min(buffer_size, file.size - file.offset); // uprava velikosti, aby se nepreteklo

    if (buffer_size <= 0) {
        return kiv_os::NOS_Error::IO_Error;
    }

    std::vector<char> buffer;
    auto res = fs->Read(file, buffer_size, file.offset, buffer);

    // prekopirovani
    for (size_t i = 0; i < buffer.size(); ++i) {
        target_buffer[i] = buffer.at(i);
    }

    bytes_read = buffer_size;
    file.offset += bytes_read; // uprava pozice

    return res;
}

/**
 * Zavre soubor
 * @return success
 */
kiv_os::NOS_Error Fs_File::Close() {
    return kiv_os::NOS_Error::Success;
}


/**
 * Provede opraci seek
 * @param position nova pozice
 * @param seek_type typ seek
 * @param seek_operation operace
 * @param res_pos vysledna pozice
 * @return vysledek oprace, uspech/neuspech
 */
kiv_os::NOS_Error
Fs_File::Seek(size_t position, kiv_os::NFile_Seek seek_type, kiv_os::NFile_Seek seek_operation, size_t &res_pos) {
    switch (seek_operation) {
        case kiv_os::NFile_Seek::Set_Size: {
            if (Is_Read_Only()) {
                return kiv_os::NOS_Error::Permission_Denied;
            }
            auto res = fs->Set_Size(file, position);
            if (res != kiv_os::NOS_Error::Success) {
                return res;
            }
            file.size = position;
            file.offset = position;
            break;
        }

        case kiv_os::NFile_Seek::Get_Position:
            res_pos = file.offset;
            break;
        default:
        case kiv_os::NFile_Seek::Set_Position:
            switch (seek_type) {
                default:
                case kiv_os::NFile_Seek::Current:
                    file.offset = std::max(file.offset + position, file.size); // aby se neprekrocila velikost souboru
                    break;
                case kiv_os::NFile_Seek::Beginning:
                    file.offset = position; // jen na danou pozici
                    break;
                case kiv_os::NFile_Seek::End:
                    file.offset = file.size - position; // od konce
                    break;
            }
    }
    return kiv_os::NOS_Error::Success;
}

Fs_File::~Fs_File() {
//    delete file.name; //TODO smazat asi
}
