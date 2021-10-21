//
// Created by Kuba on 10.10.2021.
//

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

    table = ReadFromRegisters(kFatTableSectorCount, kSectorSize, start_index);
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

    root_dir = ReadFromRegisters(kRootDirSize, kSectorSize, kRootDirSectorStart);

    return root_dir;
}

/**
 * Precte data zacinajici a koncici na danych clusterech (tzn. sektorech)
 * @param start_cluster cislo prvniho clusteru
 * @param cluster_count pocet clusteru k precteni
 * @return data z clusteru
 */
std::vector<unsigned char> ReadDataFromCluster(int start_cluster, int cluster_count) {
    std::vector<unsigned char> bytes;
    int startIndex = start_cluster + kDataSectorConversion;

    bytes = ReadFromRegisters(cluster_count, kSectorSize, startIndex);

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
std::vector<unsigned char> ReadFromRegisters(int cluster_count, int sector_size, int start_index) {
    std::vector<unsigned char> result;

    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet addressPacket;

    int size = cluster_count * sector_size;

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

// TODO koment
void WriteToRegisters(std::vector<char> buffer, int start_index) {
    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet address_packet;

    address_packet.count = buffer.size() / kSectorSize + (buffer.size() % kSectorSize);
    address_packet.lba_index = start_index + kDataSectorConversion;

    registers.rdx.l = kDiskNum;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&address_packet);


    // cast posledniho sektoru nemusi byt vzdy prepsana - mohou tam byt jina data - nechat
    int last_sector = static_cast<int>(address_packet.count) + start_index - 1;
    std::vector<unsigned char> lastSectorData = ReadFromRegisters(1, kSectorSize, last_sector);
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
    for (int i = (int)bytes.size() - 1; i >=0; i--) {
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
    res.push_back(value >> 8);
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
    WriteToRegisters(fat_char, 1 - kDataSectorConversion);
    WriteToRegisters(fat_char, 1 + kFatTableSectorCount - kDataSectorConversion);
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

    while (start < kMaxClusterNum) { //TODO mozna 4088 a to push prvni taky mozna check
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
        if (content.at(i) == 0 || content.at(i) == 246) { //TODO check a konstanta
            break;
        }

        DirItem dir_item;
        dir_item.file_name = "";

        for (int j = 0; j < kFileNameSize; ++j) {
            if (content.at(i) == ' ') { //konec nazvu souboru //TODO konstanta
                i += kFileNameSize - j; //dopocteno do 8 bytu
                break;
            } else {
                dir_item.file_name.push_back((char) content.at(i));
                i++;
            }
        }

        dir_item.file_extension = "";
        for (int j = 0; j < kFileExtensionSize; ++j) {
            if (content.at(i) == ' ') { //konec nazvu souboru //TODO konstanta
                i += kFileExtensionSize - j; //dopocteno do 8 bytu
                break;
            } else {
                dir_item.file_name.push_back((char) content.at(i));
                i++;
            }
        }

        dir_item.attribute = content.at(i++);

        i += kDirItemUnusedBytes;

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
 * @param sector_num cislo prvniho sektoru dane slozky
 * @return vektor polozek adresare v dane slozce
 */
std::vector<DirItem> GetFoldersFromDir(const std::vector<unsigned char> &fat, int sector_num) {
    if (sector_num == kRootDirSectorStart) {
        std::vector<unsigned char> data_clusters = ReadDataFromCluster(kRootDirSectorStart - kDataSectorConversion,
                                                                       kRootDirSize);

        std::vector<DirItem> content = GetDirectoryItems(data_clusters, kRootDirSize); //TODO udelat

        content.erase(content.begin());

        return content;

    } else {
        std::vector<int> sectors_indexes = GetSectorsIndexes(fat, sector_num);

        std::vector<unsigned char> data_clusters;

        std::vector<DirItem> dir_items;

        for (int i = 0; i < sectors_indexes.size(); ++i) {
            data_clusters = ReadDataFromCluster(sectors_indexes[i], 1); // obsah jednoho clusteru
            std::vector<DirItem> content = GetDirectoryItems(data_clusters, 1);

            int j = i == 0 ? 2 : 0; // prvni cluster slozky - obsahuje i '.' a '..' - preskocit

            while (j < content.size()) {
                dir_items.push_back(content.at(j));
                j++;
            }

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
        dir_item.file_name = "/";
        dir_item.file_extension = "";
        dir_item.file_size = 0;
        dir_item.attribute = static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID);
    }

    std::vector<DirItem> cur_folder_items; // polozky v prave prochazene slozce
    int directory_item_num;
    for (auto &path_part: path.path_vector) {
        cur_folder_items = GetFoldersFromDir(fat, start_cluster);

        directory_item_num = -1; // poradi slozky

        for (int j = 0; j < cur_folder_items.size(); ++j) {
            DirItem dir_item = cur_folder_items.at(j);
            std::string dir_item_full_name = dir_item.file_name; // cele jmeno vcetne pripony, pokud existuje
            if (!dir_item.file_extension.empty()) {
                dir_item_full_name += "." + dir_item.file_extension; //TODO mozna konstanta '.'
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
        if (c == '.') { //TODO const
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
    if (path.full_name == ".") { //odstrani '.' //TODO const
        path.path_vector.pop_back();
    }

    if (path.path_vector.empty()) { // root
        std::vector<unsigned char> rootContent = ReadFromRegisters(kRootDirSize, kSectorSize,
                                                                   kRootDirSectorStart - kDataSectorConversion);
        return GetDirectoryEntries(rootContent, kRootDirSize, true);
    }

    DirItem dir_item = GetDirItemCluster(kRootDirSectorStart, path, fat);

    int start_cluster = dir_item.first_cluster;

    std::vector<int> clusters_indexes = GetSectorsIndexes(fat, start_cluster);

    std::vector<unsigned char> cluster_data;
    std::vector<unsigned char> all_clusters_data;

    for (int cluster_index: clusters_indexes) {
        cluster_data = ReadFromRegisters(1, kSectorSize, cluster_index);
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

    buffer_to_write.push_back('.'); // TODO const

    // zbytek nazvu a pripony
    for (int i = 0; i < kFileNameSize + kFileExtensionSize - 1; ++i) {
        buffer_to_write.push_back(' '); // TODO const
    }

    buffer_to_write.push_back(static_cast<char>(kiv_os::NFile_Attributes::Directory));

    for (int i = 0; i < kDirItemUnusedBytes; ++i) {
        buffer_to_write.push_back(' '); //TODO const
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
        buffer_to_write.push_back('.'); // TODO const
        buffer_to_write.push_back('.'); // TODO const

        // zbytek nazvu a pripony
        for (int i = 0; i < kFileNameSize + kFileExtensionSize - 2; ++i) {
            buffer_to_write.push_back(' '); // TODO const
        }

        buffer_to_write.push_back(static_cast<char>(kiv_os::NFile_Attributes::Directory));

        for (int i = 0; i < kDirItemUnusedBytes; ++i) {
            buffer_to_write.push_back(' '); //TODO const
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

    WriteToRegisters(buffer_to_write, cur_index);
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
        if (content[i] == 0 || content[i] == 246) { //TODO check a konstanta
            break;
        }

        kiv_os::TDir_Entry dir_entry;

        int file_name_pos = 0; // pozice ukladani jmena

        for (int j = 0; j < kFileNameSize; ++j) {
            if (content[i] == ' ') { //konec nazvu souboru //TODO konstanta
                i += kFileNameSize - j; //dopocteno do 8 bytu
                file_name_pos = j;
                break;
            } else {
                dir_entry.file_name[j] = (char) content.at(i);
                i++;
            }
        }

        dir_entry.file_name[file_name_pos++] = '.'; //TODO const

        for (int j = 0; j < kFileExtensionSize; ++j) {
            if (content[i] == ' ') { //konec pripony souboru //TODO konstanta
                i += kFileExtensionSize - j; //dopocteno do 8 bytu
                break;
            } else {
                dir_entry.file_name[file_name_pos++] = (char) content.at(i);
                i++;
            }
        }

        if (file_name_pos != 12) {
            dir_entry.file_name[file_name_pos] = '\0'; //TODO const
        } else if (dir_entry.file_name[file_name_pos - 1] == '.') { //bez pripony //TODO const
            dir_entry.file_name[file_name_pos - 1] = '\0'; //TODO konst
        }

        dir_entry.file_attributes = content[i++];

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
 * @return zprava o uspechu / neuspechu
 */
kiv_os::NOS_Error CreateFileOrDir(Path &path, uint8_t attributes, std::vector<unsigned char> &fat, bool is_dir) {
    std::string folder_name = path.full_name;

    path.path_vector.pop_back(); // cesta, tzn. bez posledni polozky (jmena)

    int start_sector;

    std::vector<int> sectors_indexes;

    if (path.path_vector.empty()) { //root
        start_sector = kRootDirSectorStart;
        for (int i = kRootDirSectorStart; i < kUserDataStart; ++i) {
            sectors_indexes.push_back(i);
        }
    } else {
        DirItem target_folder = GetDirItemCluster(kRootDirSectorStart, path, fat);
        sectors_indexes = GetSectorsIndexes(fat, target_folder.first_cluster);
        start_sector = sectors_indexes.at(0);
    }

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
        buffer_to_write.push_back(' '); //TODO constanta
    }

    // pridat priponu - slozka nema zadnou - path_vector.extension.size() je 0

    buffer_to_write.insert(buffer_to_write.end(), path.extension.begin(), path.extension.end());

    for (int i = (int) path.extension.size(); i < kFileExtensionSize; i++) { //doplnit na 3 bytu
        buffer_to_write.push_back(' '); //TODO constanta
    }

    buffer_to_write.push_back(attributes); // atributy

    // nedulezite (cas vytvoreni atd.)
    for (int j = 0; j < kDirItemUnusedBytes; ++j) {
        buffer_to_write.push_back(' '); //TODO const
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

    if (path.path_vector.empty()) { // slozka v rootu
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
            int newClusterPos = AllocateNewCluster(start_sector, fat);
            if (newClusterPos == -1) { // nepovedlo se alokovat novy cluster - nelze pridat
                can_add = false;
            } else {
                sectors_indexes.push_back(newClusterPos);
            }
        }
    }

    if (can_add) { // muzu pridat tak pridam
        size_t cluster_pos = (directory_items.size() + extra_dir_item - 1) /
                             kMaxItemsPerCluster; // poradi clusteru - jeden pojme 16 polozek
        size_t item_pos = (directory_items.size() + extra_dir_item - 1) % kMaxItemsPerCluster; // poradi v ramci clusteru

        std::vector<unsigned char> cluster_data = ReadFromRegisters(1, kSectorSize, sectors_indexes.at(cluster_pos) -
                                                                                    kDataSectorConversion);

        for (int j = 0; j < buffer_to_write.size(); ++j) {
            cluster_data.at(item_pos * kDirItemSize + j) = buffer_to_write.at(j);
        }

        for (unsigned char &data: cluster_data) {
            buffer_to_save.push_back((char) data);
        }

        WriteToRegisters(buffer_to_save, sectors_indexes.at(cluster_pos) - kDataSectorConversion);
    } else { // nelze alokovat cluster
        WriteValueToFat(fat, free_index, 0); // uvolni misto
        SaveFat(fat);
        return kiv_os::NOS_Error::Not_Enough_Disk_Space;
    }

    if (is_dir) { // je slozka tak zapsat jeste '.' a pripadne '..'
        bool is_root = path.path_vector.empty(); // je root - nema '..' //TODO check
        WriteCurrentAndParentFolder(free_index, start_sector, is_root);
    }

    return kiv_os::NOS_Error::Success;
}
