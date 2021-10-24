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

/**
 * Odstrani slozku s danou cestou
 * @param path cesta k slozce
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::RmDir(Path &path) {
    DirItem dir_item = GetDirItemCluster(kRootDirSectorStart, path, fat); // cilova polozka
    if (dir_item.file_size == -1) { // nenalezeno
        return kiv_os::NOS_Error::File_Not_Found;
    }

    std::vector<int> sector_indexes = GetSectorsIndexes(fat, dir_item.first_cluster);
    std::vector<DirItem> directory_items = GetFoldersFromDir(fat, dir_item.first_cluster);

    if (!directory_items.empty()) { // nelze smazat - neni prazdna slozka
        return kiv_os::NOS_Error::Directory_Not_Empty;
    }

    std::vector<char> buffer_to_clear(kSectorSize, 0);

    for (int sector_index: sector_indexes) {
        WriteValueToFat(fat, sector_index, 0); // uvolni misto
    }

    SaveFat(fat);

    // odstraneni odkazu z nadrazene slozky
    path.DeleteNameFromPath();
    std::vector<int> sector_indexes_parent_folder;
    bool is_parent_folder_root = true;
    int start_sector = kRootDirSectorStart;
    if (path.path_vector.empty()) { // nadrazena slozka je root
        for (int i = kRootDirSectorStart; i < kUserDataStart; ++i) {
            sector_indexes_parent_folder.push_back(i);
        }
    } else { // neni root
        is_parent_folder_root = false;
        DirItem dir_item_parent = GetDirItemCluster(kRootDirSectorStart, path, fat); // cilova polozka
        sector_indexes_parent_folder = GetSectorsIndexes(fat, dir_item_parent.first_cluster);
        start_sector = sector_indexes_parent_folder.at(0);
    }

    std::vector<DirItem> directory_items_parent_folder = GetFoldersFromDir(fat, start_sector); // obsah nadrazene slozky

    int index_to_delete = -1;

    for (int i = 0; i < directory_items_parent_folder.size(); ++i) {
        DirItem cur_dir_item = directory_items_parent_folder.at(i);
        if (cur_dir_item.file_extension == path.extension && cur_dir_item.file_name == path.name) {
            index_to_delete = i;
            break;
        }
    }

    std::vector<char> folder_content;
    for (int sector_index: sector_indexes_parent_folder) {
        std::vector<unsigned char> cluster_data;
        cluster_data = ReadDataFromCluster(1, sector_index, is_parent_folder_root);
        folder_content.insert(folder_content.end(), cluster_data.begin(), cluster_data.end());
    }


    if (is_parent_folder_root) {
        index_to_delete += 1; // '.' //TODO check jestli tam je
    } else {
        index_to_delete += 2; // '.' a '..'
    }

    // odstranit nechteny a doplnit 0
    folder_content.erase(folder_content.begin() + index_to_delete * kDirItemSize,
                         folder_content.begin() + index_to_delete * kDirItemSize + kDirItemSize);

    for (int i = 0; i < kDirItemSize; ++i) {
        folder_content.push_back(0);
    }

    // nejprve vynuluje a pote doplni data do clusteru
    for (int i = 0; i < sector_indexes_parent_folder.size(); ++i) {
        std::vector<char> data_to_write;
        for (int j = 0; j < kSectorSize; ++j) {
            data_to_write.push_back(folder_content.at(i * kSectorSize + j));
        }
        WriteDataToCluster(buffer_to_clear, sector_indexes_parent_folder.at(i), is_parent_folder_root);
        WriteDataToCluster(data_to_write, sector_indexes_parent_folder.at(i), is_parent_folder_root);

    }
    return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error Fat12::Write(File file, size_t size, size_t offset, std::vector<char> buffer, size_t &written) {
    if (offset > file.size) {
        return kiv_os::NOS_Error::IO_Error;
    }

    std::vector<int> sector_indexes = GetSectorsIndexes(fat, file.handle);

    size_t sector_to_write_index = 1; // sektor, kam se budou zapisovat data

    if (offset > 0) {
        sector_to_write_index = (offset / kSectorSize) + 1;
    }

    if (sector_to_write_index > sector_indexes.size()) { // musime najit novy cluster
        int new_cluster_pos = AllocateNewCluster(sector_indexes.at(0), fat);
        if (new_cluster_pos == -1) { // nevejde se
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        }
        sector_indexes.push_back(new_cluster_pos);
    }

    int data_to_remain_last_cluster =
            static_cast<int>(offset) % kSectorSize; // tolik dat bude uchovano z posledniho clusteru

    std::vector<unsigned char> last_cluster_data = ReadDataFromCluster(1, sector_indexes.back(), false); // data posledniho clusteru
    std::vector<unsigned char> buffer_to_write;

    // puvodni data clusteru
    buffer_to_write.insert(buffer_to_write.end(), last_cluster_data.begin(), last_cluster_data.end());
    // nova data
    buffer_to_write.insert(buffer_to_write.end(), buffer.begin(), buffer.end());

    size_t bytes_written = 0 - static_cast<size_t>(data_to_remain_last_cluster);

    size_t clusters_to_write_count = buffer_to_write.size() / kSectorSize + (buffer_to_write.size() % kSectorSize !=
                                                                             0); // pocet clusteru, kam se bude zapisovat

    std::vector<char> cluster_data_to_write;

    for (int i = 0; i < clusters_to_write_count; ++i) {
        if (i == clusters_to_write_count - 1) { // cast clusteru
            cluster_data_to_write.insert(cluster_data_to_write.end(), buffer_to_write.begin() + i * kSectorSize,
                                         buffer_to_write.begin() + i * kSectorSize +
                                         (buffer_to_write.size() - i * kSectorSize));
        } else { // cely cluster
            cluster_data_to_write.insert(cluster_data_to_write.end(), buffer_to_write.begin() + i * kSectorSize,
                                         buffer_to_write.begin() + i * kSectorSize + kSectorSize);
        }

        if (sector_to_write_index + i - 1 < sector_indexes.size()) {
            WriteDataToCluster(cluster_data_to_write, sector_indexes.at(sector_to_write_index + i - 1), false);
            bytes_written += cluster_data_to_write.size();
        } else { // alokovat novy cluster
            int free_index = GetFreeIndex(fat);
            if (free_index == -1) { // nenalezen volny cluster
                SaveFat(fat);
                // pocet nove pridanych bytu - pokud offset nebyl na konci, nemuselo se pridat nic
                size_t added_bytes = (offset + bytes_written) - file.size;
                if (added_bytes > 0) {
                    ChangeFileSize(file.name, file.size + added_bytes, fat);
                    file.size += added_bytes;
                }
                written = bytes_written;
                return kiv_os::NOS_Error::Not_Enough_Disk_Space; // nebyl zapsan cely buffer
            }

            WriteValueToFat(fat, sector_indexes.back(), free_index); // na posledni zapsat cislo dalsiho clusteru
            WriteValueToFat(fat, free_index, kEndClusterInt); // posledni, oznacit konec
            WriteDataToCluster(cluster_data_to_write, free_index, false); // zapsat data
        }
        cluster_data_to_write.clear();
    }

    SaveFat(fat);

    // pocet nove pridanych bytu - pokud offset nebyl na konci, nemuselo se pridat nic
    size_t added_bytes = (offset + bytes_written) - file.size;
    if (added_bytes > 0) {
        ChangeFileSize(file.name, file.size + added_bytes, fat);
        file.size += added_bytes;
    }
    written = bytes_written;
    return kiv_os::NOS_Error::Success;
}


