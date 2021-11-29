//
// Created by Kuba on 09.10.2021.
//

#include <cstring>
#include "fat12.h"

std::vector<unsigned char> fat;
std::vector<unsigned char> second_fat;

/**
 * Nacte FAT 1 a FAT 2
 */
Fat12::Fat12() {
    fat = Fat_Helper::Load_Fat_Table(1);
    second_fat = Fat_Helper::Load_Fat_Table(1 + Fat_Helper::kFatTableSectorCount);
}

/**
 * Otevre soubor/adresar, pokud neexistuje, tak jej vytvori
 * @param path cesta k souboru/adresari
 * @param flags rezim otevreni noveho souboru
 * @param file soubor
 * @param attributes atributy
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Open(Path &path, const kiv_os::NOpen_File flags, File &file, uint8_t attributes) {
    file = File{};
    std::string file_name = path.To_String();
    size_t length = file_name.length() + 1;
    file.name = new char[length];
    strcpy_s(file.name, length, file_name.c_str());

    std::vector<std::string> pathCopy(path.path_vector); //TODO asi smazat

    Fat_Helper::DirItem dir_item = Fat_Helper::Get_Dir_Item_Cluster(Fat_Helper::kRootDirSectorStart, path, fat);

    int32_t target_cluster = dir_item.first_cluster;

    if (target_cluster == -1) { // nenalezen - tzn. neexistuje
        if (flags == kiv_os::NOpen_File::fmOpen_Always) { // musi existovat, aby byl otevren => chyba
            return kiv_os::NOS_Error::File_Not_Found;
        } else {
	        kiv_os::NOS_Error res;
	        // vytvoreni souboru
            dir_item.file_size = 0;
            dir_item.attributes = attributes;

            if (!Fat_Helper::Validate_File_Name(path.full_name)) {
                return kiv_os::NOS_Error::Invalid_Argument;
            }

            // slozka
            if (attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
                attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
                res = Mk_Dir(path, attributes);
            } else { // soubor

                file.size = 0;
                res = Create_File(path, attributes);
            }

            if (res == kiv_os::NOS_Error::Not_Enough_Disk_Space) {
                return res;
            } else {
                target_cluster = Fat_Helper::Get_Dir_Item_Cluster(Fat_Helper::kRootDirSectorStart, path,
                                                                  fat).first_cluster;
            }
        }
    } else if ((((dir_item.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) != 0) &&
                ((attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) == 0))) {
        // neni atribut slozka - chybi nastaveny bit 0x10 - ale snaha otevrit slozku
        return kiv_os::NOS_Error::Permission_Denied;
    }
    // soubor/slozka existuje (pripadne vytvoren(a))

    file.handle = target_cluster;
    file.attributes = dir_item.attributes;

    // zjistenu, jestli je to slozka
    if (dir_item.attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
        dir_item.attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
	    const std::vector<kiv_os::TDir_Entry> directory_entries = Fat_Helper::Read_Directory(path, fat);
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
kiv_os::NOS_Error Fat12::Create_File(Path &path, const uint8_t attributes) {
    return Fat_Helper::Create_File_Or_Dir(path, attributes, fat, false);
}

/**
 * Vytvori novou slozku
 * @param path cesta k slozce
 * @param attributes atributy slozky
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Mk_Dir(Path &path, const uint8_t attributes) {
    return Fat_Helper::Create_File_Or_Dir(path, attributes, fat, true);
}

/**
 * Odstrani slozku s danou cestou
 * @param path cesta k slozce
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Rm_Dir(Path &path) {
	const Fat_Helper::DirItem dir_item = Fat_Helper::Get_Dir_Item_Cluster(Fat_Helper::kRootDirSectorStart, path,
	                                                                      fat); // cilova polozka

    if (dir_item.file_size == -1) { // nenalezeno
        return kiv_os::NOS_Error::File_Not_Found;
    }

	const std::vector<int> sector_indexes = Fat_Helper::Get_Sectors_Indexes(fat, dir_item.first_cluster);
    std::vector<Fat_Helper::DirItem> directory_items = Fat_Helper::Get_Folders_From_Dir(fat, dir_item.first_cluster);

    if (!directory_items.empty()) { // nelze smazat - neni prazdna slozka
        return kiv_os::NOS_Error::Directory_Not_Empty;
    }

	const std::vector<char> buffer_to_clear(Fat_Helper::kSectorSize, 0);

    for (const int sector_index: sector_indexes) {
        Fat_Helper::Write_Value_To_Fat(fat, sector_index, 0); // uvolni misto
    }

    Fat_Helper::Save_Fat(fat);

    // odstraneni odkazu z nadrazene slozky
    path.Delete_Name_From_Path();

    std::vector<int> sector_indexes_parent_folder;
    bool is_parent_folder_root = true;
	const int start_sector = Fat_Helper::Get_Start_Sector(path, fat, is_parent_folder_root, sector_indexes_parent_folder);

	const int index_to_delete = Fat_Helper::Get_Item_Index(path, start_sector, fat);


    std::vector<char> folder_content;
    for (const int sector_index: sector_indexes_parent_folder) {
        std::vector<unsigned char> cluster_data;
        cluster_data = Fat_Helper::Read_Data_From_Cluster(1, sector_index, is_parent_folder_root);
        folder_content.insert(folder_content.end(), cluster_data.begin(), cluster_data.end());
    }

    // odstranit nechteny a doplnit 0
    folder_content.erase(folder_content.begin() + index_to_delete * Fat_Helper::kDirItemSize,
                         folder_content.begin() + index_to_delete * Fat_Helper::kDirItemSize +
                         Fat_Helper::kDirItemSize);

    for (int i = 0; i < Fat_Helper::kDirItemSize; ++i) {
        folder_content.push_back(0);
    }

    // nejprve vynuluje a pote doplni data do clusteru
    for (int i = 0; i < sector_indexes_parent_folder.size(); ++i) {
        std::vector<char> data_to_write(Fat_Helper::kSectorSize);
        for (int j = 0; j < Fat_Helper::kSectorSize; ++j) {
            data_to_write.push_back(folder_content.at(i * Fat_Helper::kSectorSize + j));
        }
        Fat_Helper::Write_Data_To_Cluster(buffer_to_clear, sector_indexes_parent_folder.at(i), is_parent_folder_root);
        Fat_Helper::Write_Data_To_Cluster(data_to_write, sector_indexes_parent_folder.at(i), is_parent_folder_root);

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
    if (path.path_vector.empty()) { // root
	    const std::vector<unsigned char> root_directory_content = Fat_Helper::Read_Data_From_Cluster(Fat_Helper::kRootDirSize,
	                                                                                                 Fat_Helper::kRootDirSectorStart,
	                                                                                                 true);
        entries = Fat_Helper::Get_Directory_Entries(root_directory_content, Fat_Helper::kRootDirSize, true);
        return kiv_os::NOS_Error::Success;
    }
    // ne root

    Fat_Helper::DirItem dir_item = Fat_Helper::Get_Dir_Item_Cluster(Fat_Helper::kRootDirSectorStart, path, fat);
    if (dir_item.first_cluster == -1) { // nenalezen
        return kiv_os::NOS_Error::File_Not_Found;
    }
    const std::vector<int> sectors_indexes = Fat_Helper::Get_Sectors_Indexes(fat, dir_item.first_cluster);

    std::vector<unsigned char> all_clusters_data;

    for (const int sectors_index: sectors_indexes) {
        std::vector<unsigned char> cluster_data = Fat_Helper::Read_Data_From_Cluster(1, sectors_index, false);
        all_clusters_data.insert(all_clusters_data.end(), cluster_data.begin(), cluster_data.end());
    }

    entries = Fat_Helper::Get_Directory_Entries(all_clusters_data, sectors_indexes.size(), false);

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
kiv_os::NOS_Error Fat12::Read(File file, const size_t bytes_to_read, const size_t offset, std::vector<char> &buffer) {
    if (((file.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) == 0) &&
        ((file.attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) == 0)) { // soubor
        if (file.size < (bytes_to_read + offset)) { // mimo rozsah velikosti souboru
            return kiv_os::NOS_Error::IO_Error;
        }

        const std::vector<int> sectors_indexes = Fat_Helper::Get_Sectors_Indexes(fat, file.handle);

        size_t cluster_index = offset / Fat_Helper::kSectorSize;  // prvni sektor pro cteni
        const size_t bytes_to_skip =
                offset %
                Fat_Helper::kSectorSize; // cast bytu v sektoru, ze ktereho se bude cist, ktere budou preskoceny

        std::vector<unsigned char> sector_data = Fat_Helper::Read_Data_From_Cluster(1, sectors_indexes.at(cluster_index),
	        false);
        for (int i = static_cast<int>(bytes_to_skip); i < Fat_Helper::kSectorSize; ++i) {
            if (bytes_to_read == buffer.size()) { // precten pocet bytu, jaky mel byt
                return kiv_os::NOS_Error::Success;
            }
            buffer.push_back(static_cast<char>(sector_data.at(i)));
        }

        // cteni dokud neni dosazen pozadovany pocet bytu
        while (bytes_to_read > buffer.size()) {
            cluster_index++; // zvyseni indexu na dalsi cluster
            sector_data = Fat_Helper::Read_Data_From_Cluster(1, sectors_indexes.at(cluster_index), false);

            for (const unsigned char byte: sector_data) {
                if (bytes_to_read == buffer.size()) { // precten pocet bytu, jaky mel byt
                    return kiv_os::NOS_Error::Success;
                }
                buffer.push_back(static_cast<char>(byte));
            }
        }
    } else { // slozka
        std::vector<kiv_os::TDir_Entry> dir_entries;

        Path path(file.name);
        const kiv_os::NOS_Error res = Read_Dir(path, dir_entries);

        std::vector<char> dir_entries_bytes;
        if (res == kiv_os::NOS_Error::Success) {
            dir_entries_bytes = Fat_Helper::Convert_Dir_Entries_To_Char_Vector(dir_entries);
        }

        if ((dir_entries.size() * sizeof(kiv_os::TDir_Entry)) >=
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
kiv_os::NOS_Error Fat12::Write(File file, const size_t offset, std::vector<char> buffer, size_t &written) {
    if (offset > file.size) {
        return kiv_os::NOS_Error::IO_Error;
    }

    std::vector<int> sector_indexes = Fat_Helper::Get_Sectors_Indexes(fat, file.handle);

    size_t sector_to_write_index = 0; // sektor, kam se budou zapisovat data

    if (offset > 0) {
        sector_to_write_index = (offset / Fat_Helper::kSectorSize);
    }

    if (sector_to_write_index >= sector_indexes.size()) { // musime najit novy cluster
	    const int new_cluster_pos = Fat_Helper::Allocate_New_Cluster(sector_indexes.at(0), fat);
        if (new_cluster_pos == -1) { // nevejde se
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        }
        sector_indexes.push_back(new_cluster_pos);
    }

    const int data_to_remain_last_cluster =
            static_cast<int>(offset) % Fat_Helper::kSectorSize; // tolik dat bude uchovano z posledniho clusteru

    std::vector<unsigned char> last_cluster_data = Fat_Helper::Read_Data_From_Cluster(1, sector_indexes.at(
                                                                                              sector_to_write_index),
                                                                                      false); // data posledniho clusteru
    std::vector<unsigned char> buffer_to_write;

    // puvodni data clusteru
    buffer_to_write.insert(buffer_to_write.end(), last_cluster_data.begin(),
                           last_cluster_data.begin() + data_to_remain_last_cluster);
    // nova data
    buffer_to_write.insert(buffer_to_write.end(), buffer.begin(), buffer.end());

    size_t bytes_written = 0 - static_cast<size_t>(data_to_remain_last_cluster);

    const size_t clusters_to_write_count =
            buffer_to_write.size() / Fat_Helper::kSectorSize + (buffer_to_write.size() % Fat_Helper::kSectorSize >
                                                                0); // pocet clusteru, kam se bude zapisovat

    std::vector<char> cluster_data_to_write;

    for (int i = 0; i < clusters_to_write_count; ++i) {
        if (i == clusters_to_write_count - 1) { // cast clusteru
            cluster_data_to_write.insert(cluster_data_to_write.end(),
                                         buffer_to_write.begin() + i * Fat_Helper::kSectorSize,
                                         buffer_to_write.begin() + i * Fat_Helper::kSectorSize +
                                         (buffer_to_write.size() - i * Fat_Helper::kSectorSize));
        } else { // cely cluster
            cluster_data_to_write.insert(cluster_data_to_write.end(),
                                         buffer_to_write.begin() + i * Fat_Helper::kSectorSize,
                                         buffer_to_write.begin() + i * Fat_Helper::kSectorSize +
                                         Fat_Helper::kSectorSize);
        }

        if (sector_to_write_index + i < sector_indexes.size()) {
            Fat_Helper::Write_Data_To_Cluster(cluster_data_to_write, sector_indexes.at(sector_to_write_index + i),
                                              false);
            bytes_written += cluster_data_to_write.size();
        } else { // alokovat novy cluster
            int free_index = Fat_Helper::Get_Free_Index(fat);
            if (free_index == -1) { // nenalezen volny cluster
                Fat_Helper::Save_Fat(fat);
                // pocet nove pridanych bytu - pokud offset nebyl na konci, nemuselo se pridat nic
                const size_t added_bytes = (offset + bytes_written) - file.size;
                if (added_bytes > 0) {
                    Fat_Helper::Change_File_Size(file.name, file.size + added_bytes, fat);
                    file.size += added_bytes;
                }
                written = bytes_written;
                return kiv_os::NOS_Error::Not_Enough_Disk_Space; // nebyl zapsan cely buffer
            }

            Fat_Helper::Write_Value_To_Fat(fat, sector_indexes.back(),
                                           free_index); // na posledni zapsat cislo dalsiho clusteru
            Fat_Helper::Write_Value_To_Fat(fat, free_index, Fat_Helper::kEndClusterInt); // posledni, oznacit konec
            Fat_Helper::Write_Data_To_Cluster(cluster_data_to_write, free_index, false); // zapsat data
            bytes_written += cluster_data_to_write.size();
            sector_indexes.push_back(free_index); // pridat ondex do indexu sektoru
        }
        cluster_data_to_write.clear();
    }

    Fat_Helper::Save_Fat(fat);

    // pocet nove pridanych bytu - pokud offset nebyl na konci, nemuselo se pridat nic
    const size_t added_bytes = (offset + bytes_written) - file.size;
    if (added_bytes > 0) {
        Fat_Helper::Change_File_Size(file.name, file.size + added_bytes, fat);
        file.size += added_bytes;
    }
    written = bytes_written;
    return kiv_os::NOS_Error::Success;
}

/**
 * Zmeni velikost danemu na pozadovanou
 * @param file soubor
 * @param new_size nova velikost
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Set_Size(const File file, const size_t new_size) {
    std::vector<int> sector_indexes = Fat_Helper::Get_Sectors_Indexes(fat, file.handle);
    const size_t sector_to_write_index = (new_size / Fat_Helper::kSectorSize); // relativni v ramci sektoru
    if (new_size < file.size) {
	    const int data_to_remain_last_cluster =
                static_cast<int>(new_size) % Fat_Helper::kSectorSize; // tolik uchovat
	    const std::vector<unsigned char> last_cluster_data = Fat_Helper::Read_Data_From_Cluster(1, sector_indexes.at(
		                                                                                            sector_to_write_index),
	                                                                                            false); // data posledniho clusteru
        // puvodni data clusteru
        std::vector<char> buffer_to_write(Fat_Helper::kSectorSize);
        for (int i = 0; i < static_cast<int>(data_to_remain_last_cluster); ++i) {
            buffer_to_write.push_back(static_cast<char>(last_cluster_data.at(i)));
        }
        for (int i = static_cast<int>(data_to_remain_last_cluster); i < Fat_Helper::kSectorSize; ++i) {
            buffer_to_write.push_back(0);
        }
        Fat_Helper::Write_Data_To_Cluster(buffer_to_write, sector_indexes.at(sector_to_write_index), false);
        Fat_Helper::Write_Value_To_Fat(fat, sector_indexes.at(sector_to_write_index), Fat_Helper::kEndClusterInt);
        // vynulovat cely
        for (int i = 0; i < static_cast<int>(data_to_remain_last_cluster); ++i) {
            buffer_to_write.at(i) = 0;
        }
        // vynulovat clustery a data ve FAT
        for (int i = static_cast<int>(sector_to_write_index) + 1; i < sector_indexes.size(); ++i) {
            Fat_Helper::Write_Data_To_Cluster(buffer_to_write, sector_indexes.at(i), false);
            Fat_Helper::Write_Value_To_Fat(fat, sector_indexes.at(i), 0);
        }
    } else {
        while (sector_to_write_index + 1 >= sector_indexes.size()) {
            int free_index = Fat_Helper::Get_Free_Index(fat); // ziska novy index
            if (free_index == -1) { // neni misto - chyba
                return kiv_os::NOS_Error::Not_Enough_Disk_Space;
            }
            Fat_Helper::Write_Value_To_Fat(fat, sector_indexes.back(),
                                           free_index); // na index zapise cislo dalsiho clusteru
            sector_indexes.push_back(free_index); // prida index do indexu
        }
        Fat_Helper::Write_Value_To_Fat(fat, sector_indexes.back(), Fat_Helper::kEndClusterInt); // zapise konec clusteru
    }
    Fat_Helper::Change_File_Size(file.name, new_size, fat);
    Fat_Helper::Save_Fat(fat); // ulozit FAT
    return kiv_os::NOS_Error::Success;
}

/**
 * Ziska atributy daneho souboru/adresare
 * @param path cesta k souboru/adresari
 * @param attributes atributy - ziskana hodnota do nich ulozena
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Get_Attributes(const Path path, uint8_t &attributes) {
    return Fat_Helper::Get_Or_Set_Attributes(path, attributes, fat, true);
}

/**
 * Nastavi atributy danemu souboru/adresari
 * @param path cesta k souboru/adresari
 * @param attributes atributy, ktere se nastavi
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error Fat12::Set_Attributes(const Path path, uint8_t attributes) {
    return Fat_Helper::Get_Or_Set_Attributes(path, attributes, fat, false);
}

/**
 * Zjisti, jestli polozka (soubor/adresar) existuje
 * @param path cesta k polozce
 * @return true pokud hledana polozka nalezena, jinak false
 */
bool Fat12::Check_If_File_Exists(Path path) {
	const int start_sector = Fat_Helper::kRootDirSectorStart; // zacatek - root
	const Fat_Helper::DirItem dir_item = Fat_Helper::Get_Dir_Item_Cluster(start_sector, path, fat);
    return dir_item.first_cluster != -1; // vysledek, jestli nalezen nebo ne
}

/**
 * Vrati adresu prvniho clusteru root sektoru
 * @return adresa prvniho clusteru root sektoru
 */
uint32_t Fat12::Get_Root_Fd() {
    return Fat_Helper::kRootDirSectorStart;
}

/**
 * Zavre dany soubor
 * @param file soubor
 * @return vysledek operace
 */
kiv_os::NOS_Error Fat12::Close(File file) {
    return kiv_os::NOS_Error::Success;
}




