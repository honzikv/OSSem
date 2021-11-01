//
// Created by Kuba on 10.10.2021.
//

#include <utility>
#include <vector>
#include <hal.h>
#include <string>
#include "fat_helper.h"
#include "path.h"
#include "api.h" //TODO cesta

/**
 * Nacte FAT tabulku na specifickem indexu (urcuje 1. / 2. tabulku)
 * @param start_index index zacatku tabulky
 * @return FAT tabulka
 */
std::vector<unsigned char> LoadFatTable(int start_index) {
    std::vector<unsigned char> table;

    table = ReadFromRegisters(kFatTableSectorCount, start_index);
    return table;
}

/**
 * Zkontroluje, zdali je obsah obou tabulek FAT totozny
 * @param first_table prvni FAT tabulka
 * @param second_table druha FAT tabulka
 * @return true pokud jsou FAT tabulky totozne, jinak false
 */
bool CheckFatConsistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table) {
    for (int i = 0; i < static_cast<int>(first_table.size()); ++i) {
        if (first_table.at(i) != second_table.at(i)) {
            return false;
        }
    }
    return true;
}

/**
 * Nacte obsah root directory nachazejici se v sektorech 19 az 32. Kazde zaznam v adresari obsahuje napr. jmeno souboru a cislo prvnoho clusteru
 * @return vektor zaznamu v adresari
 */
std::vector<unsigned char> LoadRootDirectory() {
    std::vector<unsigned char> root_dir;

    root_dir = ReadFromRegisters(kRootDirSize, kRootDirSectorStart);

    return root_dir;
}

/**
 * Precte data zacinajici a koncici na danych clusterech (tzn. sektorech)
 * @param cluster_count pocet clusteru k precteni
 * @param start_cluster cislo prvniho clusteru
 * @param is_root je root slozka
 * @return data z clusteru
 */
std::vector<unsigned char> ReadDataFromCluster(int cluster_count, int start_cluster, bool is_root) {
    std::vector<unsigned char> bytes;
    // cluster 2 je prvni a je vlastne na pozici 33 (krome root)
    int start_index = start_cluster + (is_root ? 0 : kDataSectorConversion);

    bytes = ReadFromRegisters(cluster_count, start_index);

    return bytes;
}

//TODO komentar
/**
 * Nacte data z registru
 * @param cluster_count
 * @param sector_size
 * @param start_index
 * @return
 */
std::vector<unsigned char> ReadFromRegisters(int cluster_count, int start_index) {
    std::vector<unsigned char> result;

    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet addressPacket;

    int size = cluster_count * kSectorSize;

    unsigned char sector[size]; //TODO size a typ
    addressPacket.count = cluster_count;
    addressPacket.sectors = (void *) sector;
    addressPacket.lba_index = start_index;

    registers.rdx.l = kDiskNum;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&addressPacket);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers); //TODO error

    char *buffer = reinterpret_cast<char *>(addressPacket.sectors);

    for (int i = 0; i < size; ++i) {
        result.push_back(buffer[i]);
    }
    return result;
}

/**
 * Zapise data na dany cluster
 * @param buffer data, ktera se maji zapsat
 * @param start_cluster pocatecni cluster
 * @param is_root true pokud root, jinak false
 */
void WriteDataToCluster(std::vector<char> buffer, int start_cluster, bool is_root) {
    int start_index = start_cluster + (is_root ? 0 : kDataSectorConversion);
    WriteToRegisters(std::move(buffer), start_index);
}

/**
 * Zapise data na dany index
 * @param buffer data, ktera se maji zapsat
 * @param start_index index, kam se ma zapsat
 */
void WriteToRegisters(std::vector<char> buffer, int start_index) {
    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet address_packet;

    address_packet.count = buffer.size() / kSectorSize + (buffer.size() % kSectorSize);
    address_packet.lba_index = start_index;

    registers.rdx.l = kDiskNum;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&address_packet);


    // cast posledniho sektoru nemusi byt vzdy prepsana - mohou tam byt jina data - nechat
    int last_sector = static_cast<int>(address_packet.count) + start_index - 1;
    std::vector<unsigned char> lastSectorData = ReadFromRegisters(1, last_sector);
    int keep = static_cast<int>(address_packet.count) * kSectorSize;

    int last_taken = (int) buffer.size() % kSectorSize;

    size_t start = buffer.size();
    int bytes_added = 0;
    for (size_t i = start; i < keep; ++i) {
        buffer.push_back((char) lastSectorData.at(static_cast<size_t>(last_taken) + static_cast<size_t>(bytes_added)));
        bytes_added++;
    }

    address_packet.sectors = static_cast<void *>(buffer.data());

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers); //TODO error
}

/**
 * Ziska cislo clusteru z FAT (12 bit cislo, little endian)
 * @param fat FAT
 * @param pos pozice do FAT
 * @return cislo clusteru
 */
uint16_t GetClusterNum(std::vector<unsigned char> fat, int pos) {
    int index = (int) (pos * kIndexToFatConversion);
    uint16_t cluster_num = 0;
    if (pos % 2 == 0) { // druha pulka druheho bytu + prvni byte cely
        cluster_num |= ((uint16_t) fat.at(index + 1) & 0x0F) << kBitsInBytes;
        cluster_num |= (uint16_t) fat.at(index);
    } else { // cely druhy byte + prvni pulka prvniho bytu
        cluster_num |= (uint16_t) fat.at(index + 1) << kBitsInBytesHalved;
        cluster_num |= ((uint16_t) fat.at(index) & 0xF0) >> kBitsInBytesHalved;
    }
    return cluster_num;
}

/**
 * Vypocte a vrati int hodnotu z vektoru bytu
 * @param bytes vektor bytu
 * @return int hodnota z vektoru bytu
 */
int GetIntFromCharVector(std::vector<unsigned char> bytes) {
    int res = 0;
    for (int i = (int) bytes.size() - 1; i >= 0; i--) {
        res |= (int) bytes.at(i) << (i * kBitsInBytes);
    }
    return res;
}

/**
 * Vrati vektor bytu z celociselne hodnoty
 * @param value hodnota
 * @return vektor bytu
 */
std::vector<unsigned char> GetBytesFromInt(int value) {
    std::vector<unsigned char> res;
    res.push_back(value & 0xFF); // obracene - little endian
    res.push_back((value >> kBitsInBytes) & 0xFF);
    return res;
}

/**
 * Vrati vektor bytu z celociselne hodnoty (4 byty)
 * @param value hodnota
 * @return vektor bytu
 */
std::vector<unsigned char> GetBytesFromInt4(int value) {
    std::vector<unsigned char> res;
    for (int i = 0; i < 4; ++i) { // obracene - little endian
        res.push_back((value >> i * kBitsInBytes) & 0xFF);
    }
//    res.push_back(value & 0xFF); // obracene - little endian
//    res.push_back((value >> 8) & 0xFF);
//    res.push_back((value >> 16) & 0xFF);
//    res.push_back((value >> 24) & 0xFF);
    return res;
}


/**
 * Najde a vrati ve FAT index prvniho volneho clusteru
 * @param fat FAT
 * @return index prvniho volneho clusteru, -1 pokud nenalezen
 */
uint16_t GetFreeIndex(std::vector<unsigned char> fat) {
    for (int i = 0; i < fat.size(); i += 2) {
        uint16_t cluster_num = 0;
        cluster_num |= ((uint16_t) fat.at(i + 1) & 0x0F) << kBitsInBytes;
        cluster_num |= (uint16_t) fat.at(i);
        if (cluster_num == 0) {
            return i;
        }
        cluster_num = 0;
        cluster_num |= (uint16_t) fat.at(i + 2) << kBitsInBytesHalved;
        cluster_num |= ((uint16_t) fat.at(i + 1) & 0xF0) >> kBitsInBytesHalved;
        if (cluster_num == 0) {
            return i + 1;
        }
    }
    return -1;
}

/**
 * Ulozi do FAT na danou pozici novou hodnotu
 * @param fat FAT
 * @param pos pozice
 * @param new_value nova hodnota
 */
void WriteValueToFat(std::vector<unsigned char> &fat, int pos, int new_value) {
    int index = (int) (pos * kIndexToFatConversion);
    if (pos % 2 == 0) {
        fat.at(index) = new_value & 0xFF;
        fat.at(index + 1) = (uint16_t) fat.at(index + 1) & 0xF0 | (new_value & 0xF00) >> kBitsInBytes;
    } else {
        fat.at(index) = (uint16_t) fat.at(index) & 0x0F | (new_value & 0x0F) << kBitsInBytesHalved;
        fat.at(index + 1) = (new_value & 0xFF0) >> kBitsInBytesHalved;
    }
}

/**
 * Ulozi FAT
 * @param fat FAT
 */
void SaveFat(const std::vector<unsigned char> &fat) {
    std::vector<char> fat_char;
    for (unsigned char i: fat) {
        fat_char.push_back((char) i);
    }
    WriteToRegisters(fat_char, 1); // 1.
    WriteToRegisters(fat_char, 1 + kFatTableSectorCount); // 2.
}


/**
 * Ziska a vrati seznam sektoru obsahujici dany soubor
 * @param fat FAT
 * @param start prvni sektor souboru
 * @return seznam sektoru daneho souboru
 */
std::vector<int> GetSectorsIndexes(const std::vector<unsigned char> &fat, int start) {
    std::vector<int> sectors;

    sectors.push_back(start);
    start = GetClusterNum(fat, start);

    while (start < kMaxNonEndClusterIndex) {
        sectors.push_back(start);
        start = GetClusterNum(fat, start);
    }
    return sectors;
}


/**
 * Ziska a vrati vektor polozek adresare (obsah slozky)
 * @param content vektor s byty jedne slozky
 * @param sectors_count pocet sektoru dane slozky
 * @return vektor polozek adresare
 */
std::vector<DirItem> GetDirectoryItems(std::vector<unsigned char> content, int sectors_count) {
    std::vector<DirItem> dir_content;

    int i = 0;

    while (i < kSectorSize * sectors_count) {
        if (content.at(i) == 0) { // volny a vsechny dalsi taky volne
            break;
        }

        if (content.at(i) == kFreeDirItem) { // volny
            i += kDirItemSize;
            continue;
        }

        DirItem dir_item;
        dir_item.file_name = "";

        for (int j = 0; j < kFileNameSize; ++j) {
            if (content.at(i) == kSpaceChar) { // konec nazvu souboru
                i += kFileNameSize - j; // dopocteno do 8 bytu
                break;
            } else {
                dir_item.file_name.push_back((char) content.at(i));
                i++;
            }
        }

        dir_item.extension = "";
        for (int j = 0; j < kFileExtensionSize; ++j) {
            if (content.at(i) == kSpaceChar) { // konec nazvu pripony
                i += kFileExtensionSize - j; // dopocteno do 3 bytu
                break;
            } else {
                dir_item.extension.push_back((char) content.at(i));
                i++;
            }
        }

        dir_item.attributes = content.at(i);

        i += kDirItemAttributesSize; // preskocit atributy
        i += kDirItemUnusedBytes; // preskocit nevuyzite

        std::vector<unsigned char> cluster_bytes;

        for (int j = 0; j < kDirItemClusterBytes; ++j) {
            cluster_bytes.push_back(content.at(i));
            i++;
        }

        dir_item.first_cluster = GetIntFromCharVector(cluster_bytes);

        std::vector<unsigned char> file_size_bytes;

        for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
            file_size_bytes.push_back(content.at(i));
            i++;
        }

        dir_item.file_size = GetIntFromCharVector(file_size_bytes);

        dir_content.push_back(dir_item);

    }
    return dir_content;
}

/**
 * Ziska a vrati obsah dane slozky
 * @param fat FAT
 * @param start_sector cislo prvniho sektoru dane slozky
 * @return vektor polozek adresare v dane slozce
 */
std::vector<DirItem> GetFoldersFromDir(const std::vector<unsigned char> &fat, int start_sector) {
    if (start_sector == kRootDirSectorStart) { // root
        std::vector<unsigned char> data_clusters = ReadDataFromCluster(kRootDirSize, kRootDirSectorStart, true);

        std::vector<DirItem> dir_items = GetDirectoryItems(data_clusters, kRootDirSize);

        dir_items.erase(dir_items.begin()); // TODO check jestli tam '.' je

        return dir_items;

    } else {
        std::vector<int> sectors_indexes = GetSectorsIndexes(fat, start_sector);

        std::vector<unsigned char> data_clusters;

        std::vector<DirItem> dir_items;

        for (int i = 0; i < sectors_indexes.size(); ++i) {
            data_clusters = ReadDataFromCluster(1, sectors_indexes.at(i), false); // obsah jednoho clusteru
            std::vector<DirItem> content = GetDirectoryItems(data_clusters, 1);

            dir_items.insert(dir_items.end(), content.begin() + (i == 0 ? 2 : 0), content.end()); // prvni cluster slozky - obsahuje i '.' a '..' - preskocit

        }
        return dir_items;
    }
}

/**
 * Vrati polozku adresare, na kterem zacina hledany soubor/slozka
 * @param start_cluster pocatecni cluster, kde se hleda soubor/slozka
 * @param path cesta k souboru/slozce
 * @param fat FAT
 * @return polozka adresare
 */
DirItem GetDirItemCluster(int start_cluster, const Path &path, const std::vector<unsigned char> &fat) {
    if (path.path_vector.empty()) { // root
        DirItem dir_item;
        dir_item.first_cluster = kRootDirSectorStart;
        dir_item.file_name = "/"; // TODO konst
        dir_item.extension = "";
        dir_item.file_size = 0;
        dir_item.attributes = static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID);
    }

    std::vector<DirItem> cur_folder_items; // polozky v prave prochazene slozce
    int directory_item_num;
    for (auto &path_part: path.path_vector) {
        cur_folder_items = GetFoldersFromDir(fat, start_cluster);

        directory_item_num = -1; // poradi slozky

        for (int j = 0; j < cur_folder_items.size(); ++j) {
            DirItem dir_item = cur_folder_items.at(j);
            std::string dir_item_full_name = dir_item.file_name; // cele jmeno vcetne pripony, pokud existuje
            if (!dir_item.extension.empty()) {
                dir_item_full_name += std::string(1, kCurDirChar) + dir_item.extension;
            }
            if (path_part == dir_item_full_name) {
                directory_item_num = j;
                break;
            }
        }
        if (directory_item_num == -1) { // nenalezeno
            break;
        }

        DirItem dirItem = cur_folder_items.at(directory_item_num);

        start_cluster = dirItem.first_cluster;

        if (dirItem.first_cluster == 0) { // root, takze 19
            dirItem.first_cluster = kRootDirSectorStart;
        }

    }

    DirItem dir_item{};
    if (directory_item_num == -1) { // nenalezeno
        dir_item.first_cluster = -1;
        dir_item.file_size = -1;
    } else {
        dir_item = cur_folder_items.at(directory_item_num);
        if (dir_item.first_cluster == 0) {
            dir_item.first_cluster = kRootDirSectorStart;
        }
    }
    return dir_item; //TODO neco s adresou
}

/**
 * Alokuje novy cluster souboru/slozce zacinajici na dane pozici
 * @param start_cluster prvni cluster
 * @param fat FAT
 * @return cislo clusteru / -1, pokud se nepodarilo
 */
int AllocateNewCluster(int start_cluster, std::vector<unsigned char> &fat) {
    int free_index = GetFreeIndex(fat);
    if (free_index == -1) {
        return -1;
    } else {
        std::vector<int> sectors_indexes = GetSectorsIndexes(fat, start_cluster);

        WriteValueToFat(fat, free_index, kEndClusterInt); // zapise do FAT konec clusteru
        WriteValueToFat(fat, sectors_indexes.back(), free_index); // zapise do FAT konec clusteru

        SaveFat(fat);

        return free_index;
    }
}

/**
 * Zjisti, jestli je nazev souboru validni (neni prazdny, neprekracuje meze nazev ani pripona, pripona neni prazdna po '.')
 * @param file_name nazev souboru
 * @return true pokud je nazev souboru validni, jinak false
 */
bool ValidateFileName(const std::string &file_name) {
    std::vector<char> file_name_char;
    std::vector<char> file_extension_char;

    bool is_extension = false;
    for (char c: file_name) {
        if (c == kCurDirChar) {
            is_extension = true;
            continue;
        }

        if (is_extension) {
            file_extension_char.push_back(c);
        } else {
            file_name_char.push_back(c);
        }
    }

    if (file_name_char.empty() || file_name_char.size() > kFileNameSize ||
        file_extension_char.size() > kFileExtensionSize ||
        (is_extension && file_extension_char.empty())) {
        return false;
    }

    return true;
}

/**
 * Precte dany adresar
 * @param path cesta
 * @param fat FAT
 * @return obsah adresare (vektor polozek adresare)
 */
std::vector<kiv_os::TDir_Entry> ReadDirectory(Path path, const std::vector<unsigned char> &fat) {
    if (path.full_name == std::string(1, kCurDirChar)) { //odstrani '.'
        path.DeleteNameFromPath();
    }

    if (path.path_vector.empty()) { // root
        std::vector<unsigned char> rootContent = ReadDataFromCluster(kRootDirSize, kRootDirSectorStart, true);
        return GetDirectoryEntries(rootContent, kRootDirSize, true);
    }

    DirItem dir_item = GetDirItemCluster(kRootDirSectorStart, path, fat);
    std::vector<int> clusters_indexes = GetSectorsIndexes(fat, dir_item.first_cluster);

    std::vector<unsigned char> cluster_data;
    std::vector<unsigned char> all_clusters_data;

    for (int cluster_index: clusters_indexes) {
        cluster_data = ReadDataFromCluster(1, cluster_index, false);
        all_clusters_data.insert(all_clusters_data.end(), cluster_data.begin(),
                                 cluster_data.end()); //zkopirovat prvky na konec
    }

    return GetDirectoryEntries(all_clusters_data, clusters_indexes.size(), false);
}

//TODO check co ma vsechno root
/**
 * Zapise pro slozku aktualni '.' a nadrazenou slozku '..'
 * @param cur_index index aktualni slozky
 * @param parent_index index nadrazene slozky
 * @param is_root true pokud je root - nema nadrazenou slozku
 */
void WriteCurrentAndParentFolder(int cur_index, int parent_index, bool is_root) {
    std::vector<char> buffer_to_write;

    // vytvoreni aktualni slozky '.'

    buffer_to_write.push_back(kCurDirChar);

    // zbytek nazvu a pripony
    for (int i = 0; i < kFileNameSize + kFileExtensionSize - 1; ++i) {
        buffer_to_write.push_back(kSpaceChar);
    }

    buffer_to_write.push_back(static_cast<char>(kiv_os::NFile_Attributes::Directory));

    for (int i = 0; i < kDirItemUnusedBytes; ++i) {
        buffer_to_write.push_back(kSpaceChar);
    }

    // cislo clusteru
    std::vector<unsigned char> bytes_from_cluster_num = GetBytesFromInt(cur_index);

    for (unsigned char &byte_from_int: bytes_from_cluster_num) {
        buffer_to_write.push_back((char) byte_from_int);
    }

    // velikost souboru - pro slozku 0
    for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
        buffer_to_write.push_back(0);
    }

    // vytvoreni nadrazeno slozky '..'

    if (!is_root) {
        buffer_to_write.push_back(kCurDirChar);
        buffer_to_write.push_back(kCurDirChar);

        // zbytek nazvu a pripony
        for (int i = 0; i < kFileNameSize + kFileExtensionSize - 2; ++i) {
            buffer_to_write.push_back(kSpaceChar);
        }

        buffer_to_write.push_back(static_cast<char>(kiv_os::NFile_Attributes::Directory));

        for (int i = 0; i < kDirItemUnusedBytes; ++i) {
            buffer_to_write.push_back(kSpaceChar);
        }

        if (parent_index == kRootDirSectorStart) { // root je 0
            parent_index = 0;
        }

        // cislo clusteru
        bytes_from_cluster_num = GetBytesFromInt(parent_index);

        for (unsigned char &byteFromInt: bytes_from_cluster_num) {
            buffer_to_write.push_back((char) byteFromInt);
        }

        // velikost souboru - pro slozku 0
        for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
            buffer_to_write.push_back(0);
        }

        for (int i = (int) buffer_to_write.size(); i < kSectorSize; ++i) {
            buffer_to_write.push_back(0);
        }
    }

    WriteDataToCluster(buffer_to_write, cur_index, false);
}

/**
 * Ziska a vrati vektor polozek adresare (obsah slozky) - std::vector<kiv_os::TDir_Entry>
 * @param content vektor s byty jedne slozky
 * @param clusters_count pocet sektoru dane slozky
 * @param is_root - true pokud root slozka, jinak false
 * @return vektor polozek adresare
 */
std::vector<kiv_os::TDir_Entry>
GetDirectoryEntries(std::vector<unsigned char> content, size_t clusters_count, bool is_root) {
    std::vector<kiv_os::TDir_Entry> dir_content;

    int i = 0;
    while (i < kSectorSize * clusters_count) {
        if (content.at(i) == 0) { // volny a vsechny dalsi taky volne
            break;
        }

        if (content.at(i) == kFreeDirItem) { // volny
            i += kDirItemSize;
            continue;
        }

        kiv_os::TDir_Entry dir_entry{};

        int file_name_pos = 0; // pozice ukladani jmena

        for (int j = 0; j < kFileNameSize; ++j) {
            if (content.at(i) == kSpaceChar) { // konec nazvu souboru
                i += kFileNameSize - j; // dopocteno do 8 bytu
                file_name_pos = j;
                break;
            } else {
                dir_entry.file_name[j] = (char) content.at(i);
                i++;
            }
        }

        dir_entry.file_name[file_name_pos++] = kCurDirChar;

        for (int j = 0; j < kFileExtensionSize; ++j) {
            if (content.at(i) == kSpaceChar) { // konec pripony souboru
                i += kFileExtensionSize - j; // dopocteno do 3 bytu
                file_name_pos += j;
                break;
            } else {
                dir_entry.file_name[file_name_pos++] = (char) content.at(i);
                i++;
            }
        }

        if (file_name_pos > kFileNameAndExtensionMaxSize) { // max je 12 - ukoncit //TODO check
            dir_entry.file_name[kFileNameAndExtensionMaxSize - 1] = kEndOfString;
        } else if (dir_entry.file_name[file_name_pos - 1] == kCurDirChar) { // bez pripony
            dir_entry.file_name[file_name_pos - 1] = kEndOfString;
        }

        dir_entry.file_attributes = content.at(i);

        i += kDirItemAttributesSize; // preskocit atributy
        i += kDirItemUnusedBytes; // preskocit - nedulezite informace
        i += kDirItemClusterBytes; // cislo clusteru
        i += kDirItemFileSizeBytes; // velikost souboru

        dir_content.push_back(dir_entry);
    }

    if (is_root) { // je root - odstranit odkaz na aktualni polozku // TODO check co ma vsechno root
        dir_content.erase(dir_content.begin());
    } else { // neni root - odstranit odkaz na aktualni a nadrazenou polozku
        dir_content.erase(dir_content.begin());
        dir_content.erase(dir_content.begin());
    }
    return dir_content;
}

/**
 * Vytvori soubor nebo slozku
 * @param path cesta k souboru/slozce
 * @param attributes atributy
 * @param fat FAT
 * @param is_dir true pokud se jedna o slozku
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error CreateFileOrDir(Path &path, uint8_t attributes, std::vector<unsigned char> &fat, bool is_dir) {
    std::string folder_name = path.full_name;

    path.DeleteNameFromPath(); // cesta, tzn. bez posledni polozky (jmena)

    std::vector<int> sectors_indexes;
    bool is_root;
    int start_sector = GetStartSector(path, fat, is_root, sectors_indexes);

    std::vector<DirItem> directory_items = GetFoldersFromDir(fat, start_sector);

    // nalezt 1 volny cluster
    int free_index = GetFreeIndex(fat);

    if (free_index == -1) { // neni misto
        return kiv_os::NOS_Error::Not_Enough_Disk_Space;
    } else {
        WriteValueToFat(fat, free_index, kEndClusterInt); // zapise do FAT konec clusteru
        SaveFat(fat);
    }

    std::vector<unsigned char> buffer_to_write;
    std::vector<char> buffer_to_save;

    // pridat jmeno souboru
    buffer_to_write.insert(buffer_to_write.end(), path.name.begin(), path.name.end());

    for (int i = (int) path.name.size(); i < kFileNameSize; i++) { //doplnit na 8 bytu
        buffer_to_write.push_back(kSpaceChar);
    }

    // pridat priponu - slozka nema zadnou - path_vector.extension.size() je 0

    buffer_to_write.insert(buffer_to_write.end(), path.extension.begin(), path.extension.end());

    for (int i = (int) path.extension.size(); i < kFileExtensionSize; i++) { //doplnit na 3 bytu
        buffer_to_write.push_back(kSpaceChar);
    }

    buffer_to_write.push_back(attributes); // atributy

    // nedulezite (cas vytvoreni atd.)
    for (int j = 0; j < kDirItemUnusedBytes; ++j) {
        buffer_to_write.push_back(kSpaceChar);
    }

    // cislo clusteru
    std::vector<unsigned char> bytes_from_cluster_num = GetBytesFromInt(free_index);

    for (unsigned char &byte_from_int: bytes_from_cluster_num) {
        buffer_to_write.push_back(byte_from_int);
    }

    // velikost souboru - pro slozku 0, pro novy soubor tez 0
    for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
        buffer_to_write.push_back(0);
    }

    int extra_dir_item = 1; // 1 pro soubor a novou slozku, u slozky to muze byt i vice ('.' a '..')

    bool can_add = true; // zda je misto a lze pridat


    if (is_root) { // slozka v rootu
        if (is_dir) {
            extra_dir_item += 1; // navic '.'
        }
        if ((directory_items.size() + extra_dir_item) > (sectors_indexes.size() * kMaxItemsPerCluster)) {
            can_add = false;
        }
    } else { // ne root
        if (is_dir) {
            extra_dir_item += 2; // navic '.' a '..'
        }
        if ((directory_items.size() + extra_dir_item) > (sectors_indexes.size() * kMaxItemsPerCluster)) {
            int new_cluster_pos = AllocateNewCluster(start_sector, fat);
            if (new_cluster_pos == -1) { // nepovedlo se alokovat novy cluster - nelze pridat
                can_add = false;
            } else {
                sectors_indexes.push_back(new_cluster_pos);
            }
        }
    }

    if (can_add) { // muzu pridat tak pridam
        size_t cluster_pos = (directory_items.size() + extra_dir_item - 1) /
                             kMaxItemsPerCluster; // poradi clusteru - jeden pojme 16 polozek
        size_t item_pos =
                (directory_items.size() + extra_dir_item - 1) % kMaxItemsPerCluster; // poradi v ramci clusteru

        std::vector<unsigned char> cluster_data = ReadDataFromCluster(1, sectors_indexes.at(cluster_pos), is_root);

        for (int j = 0; j < buffer_to_write.size(); ++j) {
            cluster_data.at(item_pos * kDirItemSize + j) = buffer_to_write.at(j);
        }

        for (unsigned char &data: cluster_data) {
            buffer_to_save.push_back((char) data);
        }
        WriteDataToCluster(buffer_to_save, sectors_indexes.at(cluster_pos), is_root);
    } else { // nelze alokovat cluster
        WriteValueToFat(fat, free_index, 0); // uvolni misto
        SaveFat(fat);
        return kiv_os::NOS_Error::Not_Enough_Disk_Space;
    }

    if (is_dir) { // je slozka tak zapsat jeste '.' a pripadne '..'
        is_root = path.path_vector.empty(); // je root - nema '..' //TODO check
        WriteCurrentAndParentFolder(free_index, start_sector, is_root);
    }

    return kiv_os::NOS_Error::Success;
}

/**
 * Zmeni veliksot souboru v polozce adresare
 * @param file_name cesta k souboru - neparsovana
 * @param new_size nova velikost
 * @param fat FAT
 */
void ChangeFileSize(const char *file_name, size_t new_size, const std::vector<unsigned char> &fat) {
    Path path(file_name);
    path.DeleteNameFromPath();

    std::vector<int> sectors_indexes;
    bool is_root;
    int start_sector = GetStartSector(path, fat, is_root, sectors_indexes);

    int item_index = GetItemIndex(path, start_sector, fat);

    std::vector<unsigned char> file_size_bytes = GetBytesFromInt4(new_size);

    size_t cluster_pos = item_index / kMaxItemsPerCluster; // poradi slozky
    size_t item_pos = item_index % kMaxItemsPerCluster; // poradi v ramci clusteru

    std::vector<unsigned char> data_folder;

    data_folder = ReadDataFromCluster(1, sectors_indexes.at(cluster_pos), path.path_vector.empty());
    // vlozit novou velikost
    for (int i = kDirItemFileSizePos; i < kDirItemFileSizePos + kDirItemFileSizeBytes; ++i) {
        data_folder.at(item_pos * kDirItemSize + i) = file_size_bytes.at(i - kDirItemFileSizePos);
    }
    std::vector<char> buffer_to_write;
    buffer_to_write.insert(buffer_to_write.end(), data_folder.begin(), data_folder.end());

    //TODO konverze
    WriteDataToCluster(buffer_to_write, sectors_indexes.at(cluster_pos), path.path_vector.empty());
}

/**
 * Prevede vektor struktur TDir_Entry na vektor bytu
 * @param entries vektor struktur TDir_Entry
 * @return vektor struktur TDir_Entry prevedeny na vektor bytu
 */
std::vector<char> ConvertDirEntriesToChar(std::vector<kiv_os::TDir_Entry> entries) {
    std::vector<char> res;

    for (auto &entry: entries) {
        auto const pointer = reinterpret_cast<char *>(&entry);
        res.insert(res.end(), pointer, pointer + sizeof(entry));
    }

    return res;
}

/**
 * Nastavi nebo ziska atributy daneho souboru/adresare
 * @param path cesta k souboru/adresari
 * @param attributes atributy - bud ktere se maji zapsat, nebo do nich ulozena hodnota
 * @param fat FAT
 * @param read true pokud se ctou atributy, false pokud se nastavuji
 * @return vysledek operace - uspech/neuspech
 */
kiv_os::NOS_Error GetOrSetAttributes(Path path, uint8_t &attributes, const std::vector<unsigned char> &fat, bool read) {
    path.DeleteNameFromPath(); // smazat jmeno z cesty

    std::vector<int> sectors_indexes;
    bool is_root;
    int start_sector = GetStartSector(path, fat, is_root, sectors_indexes); // cislo sektoru

    int item_index = GetItemIndex(path, start_sector, fat);

    if (item_index == -1) { // nenalezen
        return kiv_os::NOS_Error::File_Not_Found;
    }


    size_t cluster_pos = item_index / kMaxItemsPerCluster; // poradi slozky
    size_t item_pos = item_index % kMaxItemsPerCluster; // poradi v ramci clusteru

    std::vector<unsigned char> folder_data;
    folder_data = ReadDataFromCluster(1, sectors_indexes.at(cluster_pos), is_root);

    if (read) { // cteme
        attributes = folder_data.at(item_pos * kDirItemSize + kDirItemAttributesPos);
    } else { // zapisujeme
        folder_data.at(item_pos * kDirItemSize + kDirItemAttributesPos) = attributes; // zapsat atributy

        std::vector<char> buffer_to_write;
        buffer_to_write.insert(buffer_to_write.end(), folder_data.begin(), folder_data.end());
        WriteDataToCluster(buffer_to_write, sectors_indexes.at(cluster_pos), is_root);
    }

    return kiv_os::NOS_Error::Success;
}


/**
 * Ziska index prvniho sektoru daneho souboru/adresare, nastavi, jestli se nachazi v rootu a seznam sektoru obsahujici dany soubor
 * @param path cesta k souboru/adresari
 * @param fat FAT
 * @param is_root nastaveni, jestli je v rootu nebo ne
 * @param sectors_indexes nastavi seznam sektoru obsahujici dany soubor/adresar
 * @return index prvniho sektoru
 */
int GetStartSector(const Path &path, const std::vector<unsigned char> &fat, bool &is_root,
                   std::vector<int> &sectors_indexes) {
    int start_sector;
    if (path.path_vector.empty()) { // root
        is_root = true;
        start_sector = kRootDirSectorStart;
        for (int i = kRootDirSectorStart; i < kUserDataStart; ++i) {
            sectors_indexes.push_back(i);
        }
    } else {
        is_root = false;
        DirItem dir_item = GetDirItemCluster(kRootDirSectorStart, path, fat);
        sectors_indexes = GetSectorsIndexes(fat, dir_item.first_cluster);
        start_sector = sectors_indexes.at(0);
    }
    return start_sector;
}

/**
 * Vrati index souboru/adresare v polozkach adresare
 * @param path cesta k souboru/adresari
 * @param start_sector prvni sektor souboru/adresare
 * @param fat FAT
 * @return index souboru/adresare v polozkach adresare, -1 pokud nenalezen
 */
int GetItemIndex(const Path &path, int start_sector, const std::vector<unsigned char> &fat) {
    std::vector<DirItem> directory_items = GetFoldersFromDir(fat, start_sector);

    int item_index = -1;
    for (int i = 0; i < directory_items.size(); ++i) {
        // kontrola, jestli ma stejny nazev i priponu
        if (NameMatches(path, directory_items.at(i))) {
            item_index = i;
            break;
        }
    }

    if (item_index == -1) { // nenalezen
        return -1;
    }

    if (path.path_vector.empty()) {
        item_index += 1; // pridat '.' //TODO check jestli tam je
    } else {
        item_index += 2; // pridat '.' a '..'
    }

    return item_index;
}

/**
 * Zjisti, jestli se rovna cele jmeno souboru (jmeno + pripona) s celym jmenem polozky adresare
 * @param path cesta k souboru/slozce
 * @param dir_item polozka adresare
 * @return true pokud se cela jmena rovnaji, jinak false
 */
bool NameMatches(const Path &path, const DirItem& dir_item) {
    if (dir_item.extension == path.extension && dir_item.file_name == path.name) {
        return true;
    }
    return false;
}