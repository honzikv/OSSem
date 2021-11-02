//
// Created by Kuba on 09.10.2021.
//

#include "fat12.h"

std::vector<unsigned char> fat;
std::vector<unsigned char> second_fat;

/**
 * Nacte FAT 1 a FAT 2
 */
Fat12::Fat12() {
    fat = Load_Fat_Table(1);
    second_fat = Load_Fat_Table(1 + kFatTableSectorCount);
}

/**
 * Otevre soubor/adresar, pokud neexistuje, tak jej vytvori
 * @param path cesta k souboru/adresari
 * @param flags rezim otevreni noveho souboru
 * @param file soubor
 * @param attributes atributy
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Open(Path &path, kiv_os::NOpen_File flags, File &file, uint8_t attributes) {
    file = File{};
    std::string fileName = path.full_name;
    file.name = &fileName.at(0);

    std::vector<std::string> pathCopy(path.path_vector);

    //TODO jenom na konci - to by mela byt chyba '.'
    DirItem dir_item = Get_Dir_Item_Cluster(kRootDirSectorStart, path, fat);

    int32_t target_cluster = dir_item.first_cluster;

    kiv_os::NOS_Error res;

    if (target_cluster == -1) { // nenalezen - tzn. neexistuje
        if (flags == kiv_os::NOpen_File::fmOpen_Always) { // musi existovat, aby byl otevren => chyba
            return kiv_os::NOS_Error::File_Not_Found;
        } else { // vytvoreni souboru
            dir_item.file_size = 0;
            dir_item.attributes = attributes;

            // slozka
            if (attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
                attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
                if (fileName.size() > kFileNameSize) { // nazev max 8 znaku
                    return kiv_os::NOS_Error::Invalid_Argument;
                }
                res = Mk_Dir(path, attributes);
            } else { // soubor
                if (!Validate_File_Name(fileName)) {
                    return kiv_os::NOS_Error::Invalid_Argument;
                }

                file.size = 0;
                res = Create_File(path, attributes);
            }

            if (res == kiv_os::NOS_Error::Not_Enough_Disk_Space) {
                return res;
            } else {
                target_cluster = Get_Dir_Item_Cluster(kRootDirSectorStart, path, fat).first_cluster;
            }
        }
    } else if (((attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) &&
                (dir_item.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory))) == 0) {
        // neni atribut slozka - chybi nastaveny bit 0x10
        return kiv_os::NOS_Error::Permission_Denied;
    }
    // soubor/slozka existuje (pripadne vytvoren(a))

    file.handle = target_cluster;
    file.attributes = dir_item.attributes;

    // zjistenu, jestli je to slozka
    if (dir_item.attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
        dir_item.attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
        std::vector<kiv_os::TDir_Entry> directory_entries = Read_Directory(path, fat);
        dir_item.file_size = directory_entries.size() * sizeof(kiv_os::TDir_Entry);
    }
    file.size = dir_item.file_size;

    return kiv_os::NOS_Error::Success;
}

/**
 * Vytvori novy soubor
 * @param path cesta k souboru
 * @param attributes atributy souboru
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Create_File(Path &path, uint8_t attributes) {
    return Create_File_Or_Dir(path, attributes, fat, false);
}

/**
 * Vytvori novou slozku
 * @param path cesta k slozce
 * @param attributes atributy slozky
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Mk_Dir(Path &path, uint8_t attributes) {
    return Create_File_Or_Dir(path, attributes, fat, true);
}

/**
 * Odstrani slozku s danou cestou
 * @param path cesta k slozce
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Rm_Dir(Path &path) {
    DirItem dir_item = Get_Dir_Item_Cluster(kRootDirSectorStart, path, fat); // cilova polozka

    if (dir_item.file_size == -1) { // nenalezeno
        return kiv_os::NOS_Error::File_Not_Found;
    }

    std::vector<int> sector_indexes = Get_Sectors_Indexes(fat, dir_item.first_cluster);
    std::vector<DirItem> directory_items = Get_Folders_From_Dir(fat, dir_item.first_cluster);

    if (!directory_items.empty()) { // nelze smazat - neni prazdna slozka
        return kiv_os::NOS_Error::Directory_Not_Empty;
    }

    std::vector<char> buffer_to_clear(kSectorSize, 0);

    for (int sector_index: sector_indexes) {
        Write_Value_To_Fat(fat, sector_index, 0); // uvolni misto
    }

    Save_Fat(fat);

    // odstraneni odkazu z nadrazene slozky
    path.Delete_Name_From_Path();

    std::vector<int> sector_indexes_parent_folder;
    bool is_parent_folder_root = true;
    int start_sector = Get_Start_Sector(path, fat, is_parent_folder_root, sector_indexes_parent_folder);

    int index_to_delete = Get_Item_Index(path, start_sector, fat);


    std::vector<char> folder_content;
    for (int sector_index: sector_indexes_parent_folder) {
        std::vector<unsigned char> cluster_data;
        cluster_data = Read_Data_From_Cluster(1, sector_index, is_parent_folder_root);
        folder_content.insert(folder_content.end(), cluster_data.begin(), cluster_data.end());
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
        Write_Data_To_Cluster(buffer_to_clear, sector_indexes_parent_folder.at(i), is_parent_folder_root);
        Write_Data_To_Cluster(data_to_write, sector_indexes_parent_folder.at(i), is_parent_folder_root);

    }
    return kiv_os::NOS_Error::Success;
}

/**
 * Precte obsah dane slozky
 * @param path cesta k slozce
 * @param entries vektor TDir_Entry - bude zmenen(naplnen obsahem adresare)
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Read_Dir(Path &path, std::vector<kiv_os::TDir_Entry> &entries) {
    if (path.path_vector.back() == std::string(1, kCurDirChar)) {
        path.Delete_Name_From_Path(); // pokud jmeno pouze '.', tak odstranit
    }

    if (path.path_vector.empty()) { // root
        std::vector<unsigned char> root_directory_content = Read_Data_From_Cluster(kRootDirSize, kRootDirSectorStart,
                                                                                   true);
        entries = Get_Directory_Entries(root_directory_content, kRootDirSize, true);
        return kiv_os::NOS_Error::Success;
    }
    // ne root

    DirItem dir_item = Get_Dir_Item_Cluster(kRootDirSectorStart, path, fat);
    if (dir_item.first_cluster == -1) { // nenalezen
        return kiv_os::NOS_Error::File_Not_Found;
    }
    std::vector<int> sectors_indexes = Get_Sectors_Indexes(fat, dir_item.first_cluster);

    //TODO tohle taky nekde asi uz pouzito - do metody
    std::vector<unsigned char> cluster_data;
    std::vector<unsigned char> all_clusters_data;

    for (int sectors_index: sectors_indexes) {
        cluster_data = Read_Data_From_Cluster(1, sectors_index, false);
        all_clusters_data.insert(all_clusters_data.end(), cluster_data.begin(), cluster_data.end());
    }

    entries = Get_Directory_Entries(all_clusters_data, sectors_indexes.size(), false);

    return kiv_os::NOS_Error::Success;
    //TODO mozna check jestli vubec existuje, nebo si to udela volajici
}

/**
 * Precte ze souboru pozadovany pocet bytu z daneho offsetu do bufferu
 * @param file soubor
 * @param bytes_to_read pocet buty k precteni
 * @param offset offset v souboru
 * @param buffer buffer, kam bude ulozen vysledek cteni
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Read(File file, size_t bytes_to_read, size_t offset, std::vector<char> &buffer) {
    if (((file.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) == 0) &&
        ((file.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) == 0)) { // soubor
        if (file.size < (bytes_to_read + offset)) { // mimo rozsah velikosti souboru
            return kiv_os::NOS_Error::IO_Error;
        }

        std::vector<int> sectors_indexes = Get_Sectors_Indexes(fat, file.handle);

        size_t cluster_index = offset / kSectorSize;  // prvni sektor pro cteni
        size_t bytes_to_skip =
                offset % kSectorSize; // cast bytu v sektoru, ze ktereho se bude cist, ktere budou preskoceny
        std::vector<unsigned char> sector_data;

        sector_data = Read_Data_From_Cluster(1, sectors_indexes.at(cluster_index), false);
        for (int i = (int) bytes_to_skip; i < kSectorSize; ++i) {
            if (bytes_to_read == buffer.size()) { // precten pocet bytu, jaky mel byt
                return kiv_os::NOS_Error::Success;
            }
            buffer.push_back((char) sector_data.at(i));
        }

        // cteni dokud neni dosazen pozadovany pocet bytu
        while (bytes_to_read > buffer.size()) {
            cluster_index++; // zvyseni indexu na dalsi cluster
            sector_data = Read_Data_From_Cluster(1, sectors_indexes.at(cluster_index), false);

            for (unsigned char byte: sector_data) {
                if (bytes_to_read == buffer.size()) { // precten pocet bytu, jaky mel byt
                    return kiv_os::NOS_Error::Success;
                }
                buffer.push_back((char) byte);
            }
        }
    } else { // slozka
        std::vector<kiv_os::TDir_Entry> dir_entries;

        Path path(file.name);
        kiv_os::NOS_Error res = Read_Dir(path, dir_entries);

        std::vector<char> dir_entries_bytes;
        if (res == kiv_os::NOS_Error::Success) {
            dir_entries_bytes = Convert_Dir_Entries_To_Char_Vector(dir_entries);
        }

        if ((dir_entries_bytes.size() * sizeof(kiv_os::TDir_Entry)) >=
            (bytes_to_read + offset)) { // lze precist, nepresahl se rozsah
            for (int i = 0; i < bytes_to_read; ++i) {
                buffer.push_back(dir_entries_bytes.at(i + offset));
            }
            return kiv_os::NOS_Error::Success;
        } else {
            return kiv_os::NOS_Error::IO_Error;
        }
    }
    return kiv_os::NOS_Error::Success;
}

/**
 * Zapise do daneho souboru na dany offset data z bufferu
 * @param file soubor
 * @param offset offset, na ktery se bude zapisovat
 * @param buffer buffer, jehoz data maji by zapsana
 * @param written pocet skutecne zapsanych bytu (ulozeno pozdeji)
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Write(File file, size_t offset, std::vector<char> buffer, size_t &written) {
    if (offset > file.size) {
        return kiv_os::NOS_Error::IO_Error;
    }

    std::vector<int> sector_indexes = Get_Sectors_Indexes(fat, file.handle);

    size_t sector_to_write_index = 1; // sektor, kam se budou zapisovat data

    if (offset > 0) {
        sector_to_write_index = (offset / kSectorSize) + 1;
    }

    if (sector_to_write_index > sector_indexes.size()) { // musime najit novy cluster
        int new_cluster_pos = Allocate_New_Cluster(sector_indexes.at(0), fat);
        if (new_cluster_pos == -1) { // nevejde se
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        }
        sector_indexes.push_back(new_cluster_pos);
    }

    int data_to_remain_last_cluster =
            static_cast<int>(offset) % kSectorSize; // tolik dat bude uchovano z posledniho clusteru

    std::vector<unsigned char> last_cluster_data = Read_Data_From_Cluster(1, sector_indexes.back(),
                                                                          false); // data posledniho clusteru
    std::vector<unsigned char> buffer_to_write;

    // puvodni data clusteru
    buffer_to_write.insert(buffer_to_write.end(), last_cluster_data.begin(),
                           last_cluster_data.begin() + data_to_remain_last_cluster);
    // nova data
    buffer_to_write.insert(buffer_to_write.end(), buffer.begin(), buffer.end());

    size_t bytes_written = 0 - static_cast<size_t>(data_to_remain_last_cluster);

    size_t clusters_to_write_count = buffer_to_write.size() / kSectorSize + (buffer_to_write.size() % kSectorSize >
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
            Write_Data_To_Cluster(cluster_data_to_write, sector_indexes.at(sector_to_write_index + i - 1), false);
            bytes_written += cluster_data_to_write.size();
        } else { // alokovat novy cluster
            int free_index = Get_Free_Index(fat);
            if (free_index == -1) { // nenalezen volny cluster
                Save_Fat(fat);
                // pocet nove pridanych bytu - pokud offset nebyl na konci, nemuselo se pridat nic
                size_t added_bytes = (offset + bytes_written) - file.size;
                if (added_bytes > 0) {
                    Change_File_Size(file.name, file.size + added_bytes, fat);
                    file.size += added_bytes;
                }
                written = bytes_written;
                return kiv_os::NOS_Error::Not_Enough_Disk_Space; // nebyl zapsan cely buffer
            }

            Write_Value_To_Fat(fat, sector_indexes.back(), free_index); // na posledni zapsat cislo dalsiho clusteru
            Write_Value_To_Fat(fat, free_index, kEndClusterInt); // posledni, oznacit konec
            Write_Data_To_Cluster(cluster_data_to_write, free_index, false); // zapsat data
        }
        cluster_data_to_write.clear();
    }

    Save_Fat(fat);

    // pocet nove pridanych bytu - pokud offset nebyl na konci, nemuselo se pridat nic
    size_t added_bytes = (offset + bytes_written) - file.size;
    if (added_bytes > 0) {
        Change_File_Size(file.name, file.size + added_bytes, fat);
        file.size += added_bytes;
    }
    written = bytes_written;
    return kiv_os::NOS_Error::Success;
}

/**
 * Ziska atributy daneho souboru/adresare
 * @param path cesta k souboru/adresari
 * @param attributes atributy - ziskana hodnota do nich ulozena
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Get_Attributes(Path path, uint8_t &attributes) {
    return Get_Or_Set_Attributes(path, attributes, fat, true);
}

/**
 * Nastavi atributy danemu souboru/adresari
 * @param path cesta k souboru/adresari
 * @param attributes atributy, ktere se nastavi
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Set_Attributes(Path path, uint8_t attributes) {
    return Get_Or_Set_Attributes(path, attributes, fat, false);
}

/**
 * Zjisti, jestli polozka (soubor/adresar) existuje
 * @param path cesta k polozce
 * @param current_fd v jakem adresari ma byt polozka hledana
 * @param target_fd zde bude ulozeno cislo polozky adresare (prvni cluster), kde se hledana polozka nachazi
 * @param root true pokud se ma hledat od root (pak je "current_fd" ignorovan), jinak false
 * @return true pokud hledana polozka nalezena, jinak false
 */
bool Fat12::File_Exists(Path path, int32_t current_fd, int32_t &target_fd, bool root) {
    if (path.full_name == std::string(1, kCurDirChar)) { // aktualni slozka - vzdy existuje
        target_fd = current_fd;
        return true;
    }

    if (path.full_name == (std::string() + kCurDirChar + kCurDirChar)) { // nadrazeny adresar
        if (root) { // root nema nadrazenou slozku
            return false;
        }
        std::vector<unsigned char> first_cluster_data = Read_Data_From_Cluster(1, current_fd, false);
        std::vector<unsigned char> first_cluster;
        // prvni je '.', druhy je '..', preskocit na spravnou pozici
        first_cluster.insert(first_cluster.end(), first_cluster_data.begin() + kDirItemSize + kDirItemClusterPos,
                             first_cluster_data.begin() + kDirItemSize + kDirItemClusterPos + kDirItemClusterBytes);

        target_fd = Get_Int_From_Char_Vector(first_cluster); // prvni cluster nadrazene slozky
        return true;
    }

    int start_sector = root ? kRootDirSectorStart : current_fd; // zacatek - root nebo soucasny
    DirItem dir_item = Get_Dir_Item_Cluster(start_sector, path, fat);
    target_fd = dir_item.first_cluster;
    return dir_item.first_cluster != -1; // vysledek, jestli naleze nebo ne
}

/**
 * Vrati adresu prvniho clusteru root sektoru
 * @return adresa prvniho clusteru root sektoru
 */
uint32_t Fat12::Get_Root_Fd() {
    return kRootDirSectorStart;
}




