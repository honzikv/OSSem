//
// Created by Kuba on 09.10.2021.
//

#include "fat12.h"

std::vector<unsigned char> fat;
std::vector<unsigned char> second_fat;


Fat12::Fat12() {
    fat = LoadFatTable(1);
    second_fat = LoadFatTable(1 + kFatTableSectorCount);
}

//TODO dodelat vse

kiv_os::NOS_Error Fat12::Open(Path &path, kiv_os::NOpen_File flags, File &file, uint8_t attributes) {
    file = File{};
    std::string fileName = path.full_name;
    file.name = &fileName.at(0);

    std::vector<std::string> pathCopy(path.path_vector);

    //TODO jenom na konci - to by mela byt chyba '.'
    DirItem dir_item = GetDirItemCluster(kRootDirSectorStart, path, fat);

    int32_t target_cluster = dir_item.first_cluster;

    kiv_os::NOS_Error res;

    if (target_cluster == -1) { // nenalezen - tzn. neexistuje
        if (flags == kiv_os::NOpen_File::fmOpen_Always) { // musi existovat, aby byl otebrem => chyba
            return kiv_os::NOS_Error::File_Not_Found;
        } else { // vytvoreni souboru
            dir_item.file_size = 0;
            dir_item.attribute = attributes;

            // slozka
            if (attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
                attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
                if (fileName.size() > kFileNameSize) { // nazev max 8 znaku
                    return kiv_os::NOS_Error::Invalid_Argument;
                }
                res = MkDir(path, attributes);
            } else { // soubor
                if (!ValidateFileName(fileName)) {
                    return kiv_os::NOS_Error::Invalid_Argument;
                }

                file.size = 0;
                res = CreateFile(path, attributes);
            }

            if (res == kiv_os::NOS_Error::Not_Enough_Disk_Space) {
                return res;
            } else {
                target_cluster = GetDirItemCluster(kRootDirSectorStart, path, fat).first_cluster;
            }
        }
    } else if (((attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) &&
                (dir_item.attribute & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory))) == 0) {
        // neni atribut slozka - chybi nastaveny bit 0x10
        return kiv_os::NOS_Error::Permission_Denied;
    }
    // soubor/slozka existuje (pripadne vytvoren(a))

    file.handle = target_cluster;
    file.attributes = dir_item.attribute;

    // zjistenu, jestli je to slozka
    if (dir_item.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
        dir_item.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
        std::vector<kiv_os::TDir_Entry> directory_entries = ReadDirectory(path, fat);
        dir_item.file_size = directory_entries.size() * sizeof(kiv_os::TDir_Entry);
    }
    file.size = dir_item.file_size;

    return kiv_os::NOS_Error::Success;
}

/**
 * Vytvori novy soubor
 * @param path cesta k souboru
 * @param attributes atributy souboru
 * @return zprava o uspechu / neuspechu
 */
kiv_os::NOS_Error Fat12::CreateFile(Path &path, uint8_t attributes) {
    return CreateFileOrDir(path, attributes, fat, false);
}

/**
 * Vytvori novou slozku
 * @param path cesta k slozce
 * @param attributes atributy slozky
 * @return zprava o uspechu / neuspechu
 */
kiv_os::NOS_Error Fat12::MkDir(Path &path, uint8_t attributes) {
    return CreateFileOrDir(path, attributes, fat, true);
}


