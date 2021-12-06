//
// Created by Kuba on 10.10.2021.
//

#include <utility>
#include <vector>
#include <string>
#include <cmath>
#include "../../api/hal.h"
#include "../../api/api.h"
#include "fat_helper.h"
#include "path.h"

namespace Fat_Helper {
    /**
     * Nacte FAT tabulku na specifickem indexu (urcuje 1. / 2. tabulku)
     * @param start_index index zacatku tabulky
     * @return FAT tabulka
     */
    std::vector<unsigned char> Load_Fat_Table(const int start_index) {

	    std::vector<unsigned char> table = Read_From_Registers(kFatTableSectorCount, start_index);
        return table;
    }

    /**
     * Zkontroluje, zdali je obsah obou tabulek FAT totozny
     * @param first_table prvni FAT tabulka
     * @param second_table druha FAT tabulka
     * @return true pokud jsou FAT tabulky totozne, jinak false
     */
    bool Check_Fat_Consistency(const std::vector<unsigned char>& first_table, const std::vector<unsigned char>& second_table) {
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
    std::vector<unsigned char> Load_Root_Directory() {

	    std::vector<unsigned char> root_dir = Read_From_Registers(kRootDirSize, kRootDirSectorStart);

        return root_dir;
    }

    /**
     * Precte data zacinajici a koncici na danych clusterech (tzn. sektorech)
     * @param cluster_count pocet clusteru k precteni
     * @param start_cluster cislo prvniho clusteru
     * @param is_root je root slozka
     * @return data z clusteru
     */
    std::vector<unsigned char> Read_Data_From_Cluster(const int cluster_count, const int start_cluster, const bool is_root) {
	    // cluster 2 je prvni a je vlastne na pozici 33 (krome root)
	    const int start_index = start_cluster + (is_root ? 0 : kDataSectorConversion);

        std::vector<unsigned char> bytes = Read_From_Registers(cluster_count, start_index);

        return bytes;
    }

    /**
     * Nacte data z registru
     * @param cluster_count pocet clusteru, ktere se budou cist
     * @param start_index index prvniho clusteru
     * @return prectene byty
     */
    std::vector<unsigned char> Read_From_Registers(const int cluster_count, const int start_index) {
        std::vector<unsigned char> result;

        kiv_hal::TRegisters registers{};
        kiv_hal::TDisk_Address_Packet address_packet{};

        const int size = cluster_count * kSectorSize;

        std::vector<unsigned char> sector_vec(size);
        unsigned char *sector = sector_vec.data();
        address_packet.count = cluster_count;
        address_packet.sectors = static_cast<void*>(sector);
        address_packet.lba_index = start_index;

        registers.rdx.l = kDiskNum;
        registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
        registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&address_packet);

        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers);

        char *buffer = reinterpret_cast<char *>(address_packet.sectors);

        result.reserve(size);
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
    void Write_Data_To_Cluster(std::vector<char> buffer, const int start_cluster, const bool is_root) {
	    const int start_index = start_cluster + (is_root ? 0 : kDataSectorConversion);
        Write_To_Registers(std::move(buffer), start_index);
    }

    /**
     * Zapise data na dany index
     * @param buffer data, ktera se maji zapsat
     * @param start_index index, kam se ma zapsat
     */
    void Write_To_Registers(std::vector<char> buffer, const int start_index) {
        kiv_hal::TRegisters registers{};
        kiv_hal::TDisk_Address_Packet address_packet{};

        address_packet.count = buffer.size() / kSectorSize + (buffer.size() % kSectorSize > 0);
        address_packet.lba_index = start_index;

        registers.rdx.l = kDiskNum;
        registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
        registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&address_packet);


        // cast posledniho sektoru nemusi byt vzdy prepsana - mohou tam byt jina data - nechat
        const int last_sector = static_cast<int>(address_packet.count) + start_index - 1;
        const std::vector<unsigned char> last_sector_data = Read_From_Registers(1, last_sector);
        const int keep = static_cast<int>(address_packet.count) * kSectorSize;

        const int last_taken = static_cast<int>(buffer.size()) % kSectorSize;

        const size_t start = buffer.size();
        int bytes_added = 0;
        for (size_t i = start; i < keep; ++i) {
            buffer.push_back(
                    static_cast<char>(last_sector_data.at(static_cast<size_t>(last_taken) + static_cast<size_t>(bytes_added))));
            bytes_added++;
        }

        address_packet.sectors = static_cast<void *>(buffer.data());

        kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers);
    }

    /**
     * Ziska cislo clusteru z FAT (12 bit cislo, little endian)
     * @param fat FAT
     * @param pos pozice do FAT
     * @return cislo clusteru
     */
    uint16_t Get_Cluster_Num(const std::vector<unsigned char>& fat, const int pos) {
	    const int index = static_cast<int>(pos * kIndexToFatConversion);
        uint16_t cluster_num = 0;
        if (pos % 2 == 0) { // druha pulka druheho bytu + prvni byte cely
            cluster_num |= (static_cast<uint16_t>(fat.at(index + 1)) & 0x0F) << kBitsInBytes;
            cluster_num |= static_cast<uint16_t>(fat.at(index));
        } else { // cely druhy byte + prvni pulka prvniho bytu
            cluster_num |= static_cast<uint16_t>(fat.at(index + 1)) << kBitsInBytesHalved;
            cluster_num |= (static_cast<uint16_t>(fat.at(index)) & 0xF0) >> kBitsInBytesHalved;
        }
        return cluster_num;
    }

    /**
     * Vypocte a vrati int hodnotu z vektoru bytu
     * @param bytes vektor bytu
     * @return int hodnota z vektoru bytu
     */
    int Get_Int_From_Char_Vector(const std::vector<unsigned char>& bytes) {
        int res = 0;
        for (int i = static_cast<int>(bytes.size()) - 1; i >= 0; i--) {
            res |= static_cast<int>(bytes.at(i)) << (i * kBitsInBytes);
        }
        return res;
    }

    /**
     * Vrati vektor bytu z celociselne hodnoty
     * @param value hodnota
     * @return vektor bytu
     */
    std::vector<unsigned char> Get_Bytes_From_Int(int value) {
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
    std::vector<unsigned char> Get_Bytes_From_Int_4(const int value) {
        std::vector<unsigned char> res;
        res.reserve(4);
        for (int i = 0; i < 4; ++i) { // obracene - little endian
            res.push_back((value >> i * kBitsInBytes) & 0xFF);
        }
        return res;
    }


    /**
     * Najde a vrati ve FAT index prvniho volneho clusteru
     * @param fat FAT
     * @return index prvniho volneho clusteru, -1 pokud nenalezen
     */
    uint16_t Get_Free_Index(const std::vector<unsigned char>& fat) {
        for (int i = 0; i < fat.size(); i += 3) {
            uint16_t cluster_num = 0;
            cluster_num |= (static_cast<uint16_t>(fat.at(i + 1)) & 0x0F) << kBitsInBytes;
            cluster_num |= static_cast<uint16_t>(fat.at(i));
            if (cluster_num == 0) {
                return ceil(i / kIndexToFatConversion);
            }
            cluster_num = 0;
            cluster_num |= static_cast<uint16_t>(fat.at(i + 2)) << kBitsInBytesHalved;
            cluster_num |= (static_cast<uint16_t>(fat.at(i + 1)) & 0xF0) >> kBitsInBytesHalved;
            if (cluster_num == 0) {
                return ceil((i + 1) / kIndexToFatConversion);
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
    void Write_Value_To_Fat(std::vector<unsigned char> &fat, const int pos, int new_value) {
	    const int index = static_cast<int>(pos * kIndexToFatConversion);
        if (pos % 2 == 0) {
            fat.at(index) = new_value & 0xFF;
            fat.at(index + 1) = static_cast<uint16_t>(fat.at(index + 1)) & 0xF0 | (new_value & 0xF00) >> kBitsInBytes;
        } else {
            fat.at(index) = static_cast<uint16_t>(fat.at(index)) & 0x0F | (new_value & 0x0F) << kBitsInBytesHalved;
            fat.at(index + 1) = (new_value & 0xFF0) >> kBitsInBytesHalved;
        }
    }

    /**
     * Ulozi FAT
     * @param fat FAT
     */
    void Save_Fat(const std::vector<unsigned char> &fat) {
        std::vector<char> fat_char;
        fat_char.reserve(fat.size());
        for (const unsigned char i: fat) {
            fat_char.push_back(static_cast<char>(i));
        }
        Write_To_Registers(fat_char, 1); // 1.
        Write_To_Registers(fat_char, 1 + kFatTableSectorCount); // 2.
    }


    /**
     * Ziska a vrati seznam sektoru obsahujici dany soubor
     * @param fat FAT
     * @param start prvni sektor souboru
     * @return seznam sektoru daneho souboru
     */
    std::vector<int> Get_Sectors_Indexes(const std::vector<unsigned char> &fat, int start) {
        std::vector<int> sectors;

        sectors.push_back(start);
        start = Get_Cluster_Num(fat, start);

        while (start < kMaxNonEndClusterIndex) {
            sectors.push_back(start);
            start = Get_Cluster_Num(fat, start);
        }
        return sectors;
    }


    /**
     * Ziska a vrati vektor polozek adresare (obsah slozky)
     * @param content vektor s byty jedne slozky
     * @param sectors_count pocet sektoru dane slozky
     * @return vektor polozek adresare
     */
    std::vector<DirItem> Get_Directory_Items(const std::vector<unsigned char>& content, const int sectors_count) {
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
                    dir_item.file_name.push_back(static_cast<char>(content.at(i)));
                    i++;
                }
            }

            dir_item.extension = "";
            for (int j = 0; j < kFileExtensionSize; ++j) {
                if (content.at(i) == kSpaceChar) { // konec nazvu pripony
                    i += kFileExtensionSize - j; // dopocteno do 3 bytu
                    break;
                } else {
                    dir_item.extension.push_back(static_cast<char>(content.at(i)));
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

            dir_item.first_cluster = Get_Int_From_Char_Vector(cluster_bytes);

            std::vector<unsigned char> file_size_bytes;

            for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
                file_size_bytes.push_back(content.at(i));
                i++;
            }

            dir_item.file_size = Get_Int_From_Char_Vector(file_size_bytes);

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
    std::vector<DirItem> Get_Folders_From_Dir(const std::vector<unsigned char> &fat, const int start_sector) {
        if (start_sector == kRootDirSectorStart) { // root
	        const std::vector<unsigned char> data_clusters = Read_Data_From_Cluster(kRootDirSize, kRootDirSectorStart, true);

            std::vector<DirItem> dir_items = Get_Directory_Items(data_clusters, kRootDirSize);

            dir_items.erase(dir_items.begin());

            return dir_items;

        } else {
	        const std::vector<int> sectors_indexes = Get_Sectors_Indexes(fat, start_sector);

	        std::vector<DirItem> dir_items;

            for (int i = 0; i < sectors_indexes.size(); ++i) {
                std::vector<unsigned char> data_clusters = Read_Data_From_Cluster(1, sectors_indexes.at(i), false); // obsah jednoho clusteru
                std::vector<DirItem> content = Get_Directory_Items(data_clusters, 1);

                // prvni cluster slozky - obsahuje i '.' a '..' - preskocit
                int j = i == 0 ? 2 : 0;
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
    DirItem Get_Dir_Item_Cluster(int start_cluster, const Path &path, const std::vector<unsigned char> &fat) {
        if (path.path_vector.empty()) { // root
            DirItem dir_item;
            dir_item.first_cluster = kRootDirSectorStart;
            dir_item.file_name = "\\";
            dir_item.extension = "";
            dir_item.file_size = 0;
            dir_item.attributes = static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID);
            return dir_item;
        }

        std::vector<DirItem> cur_folder_items; // polozky v prave prochazene slozce
        int directory_item_num = -1;
        for (auto &path_part: path.path_vector) {
            cur_folder_items = Get_Folders_From_Dir(fat, start_cluster);

            directory_item_num = -1; // poradi slozky

            for (int j = 0; j < cur_folder_items.size(); ++j) {
                DirItem dir_item = cur_folder_items.at(j);
                std::string dir_item_full_name = dir_item.file_name; // cele jmeno vcetne pripony, pokud existuje
                if (!dir_item.extension.empty()) {
                    dir_item_full_name += kCurDirChar + dir_item.extension;
                }
                if (path_part == dir_item_full_name) {
                    directory_item_num = j;
                    break;
                }
            }
            if (directory_item_num == -1) { // nenalezeno
                break;
            }

            DirItem dir_item = cur_folder_items.at(directory_item_num);

            start_cluster = dir_item.first_cluster;

            if (dir_item.first_cluster == 0) { // root, takze 19
                dir_item.first_cluster = kRootDirSectorStart;
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
        return dir_item;
    }

    /**
     * Alokuje novy cluster souboru/slozce zacinajici na dane pozici
     * @param start_cluster prvni cluster
     * @param fat FAT
     * @return cislo clusteru / -1, pokud se nepodarilo
     */
    int Allocate_New_Cluster(const int start_cluster, std::vector<unsigned char> &fat) {
	    const int free_index = Get_Free_Index(fat);
        if (free_index == -1) {
            return -1;
        } else {
	        const std::vector<int> sectors_indexes = Get_Sectors_Indexes(fat, start_cluster);

            Write_Value_To_Fat(fat, free_index, kEndClusterInt); // zapise do FAT konec clusteru
            Write_Value_To_Fat(fat, sectors_indexes.back(), free_index); // zapise do FAT konec clusteru

            Save_Fat(fat);

            return free_index;
        }
    }

    /**
     * Zjisti, jestli je nazev souboru validni (neni prazdny, neprekracuje meze nazev ani pripona, pripona neni prazdna po '.')
     * @param file_name nazev souboru
     * @return true pokud je nazev souboru validni, jinak false
     */
    bool Validate_File_Name(const std::string &file_name) {
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
    std::vector<kiv_os::TDir_Entry> Read_Directory(const Path& path, const std::vector<unsigned char> &fat) {
        if (path.path_vector.empty()) { // root
	        const std::vector<unsigned char> root_content = Read_Data_From_Cluster(kRootDirSize, kRootDirSectorStart, true);
            return Get_Directory_Entries(root_content, kRootDirSize, true);
        }

        const DirItem dir_item = Get_Dir_Item_Cluster(kRootDirSectorStart, path, fat);
        const std::vector<int> clusters_indexes = Get_Sectors_Indexes(fat, dir_item.first_cluster);

        std::vector<unsigned char> all_clusters_data;

        for (const int cluster_index: clusters_indexes) {
            std::vector<unsigned char> cluster_data = Read_Data_From_Cluster(1, cluster_index, false);
            all_clusters_data.insert(all_clusters_data.end(), cluster_data.begin(),
                                     cluster_data.end()); //zkopirovat prvky na konec
        }

        return Get_Directory_Entries(all_clusters_data, clusters_indexes.size(), false);
    }

    /**
     * Zapise pro slozku aktualni '.' a nadrazenou slozku '..'
     * @param cur_index index aktualni slozky
     * @param parent_index index nadrazene slozky
     */
    void Write_Current_And_Parent_Folder(const int cur_index, int parent_index) {
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
        std::vector<unsigned char> bytes_from_cluster_num = Get_Bytes_From_Int(cur_index);

        for (const unsigned char &byte_from_int: bytes_from_cluster_num) {
            buffer_to_write.push_back(static_cast<char>(byte_from_int));
        }

        // velikost souboru - pro slozku 0
        for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
            buffer_to_write.push_back(0);
        }

        // vytvoreni nadrazene slozky '..'

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
        bytes_from_cluster_num = Get_Bytes_From_Int(parent_index);

        for (const unsigned char &byteFromInt: bytes_from_cluster_num) {
            buffer_to_write.push_back(static_cast<char>(byteFromInt));
        }

        // velikost souboru - pro slozku 0
        for (int j = 0; j < kDirItemFileSizeBytes; ++j) {
            buffer_to_write.push_back(0);
        }

        for (int i = static_cast<int>(buffer_to_write.size()); i < kSectorSize; ++i) {
            buffer_to_write.push_back(0);
        }

        Write_Data_To_Cluster(buffer_to_write, cur_index, false);
    }

    /**
     * Ziska a vrati vektor polozek adresare (obsah slozky) - std::vector<kiv_os::TDir_Entry>
     * @param content vektor s byty jedne slozky
     * @param clusters_count pocet sektoru dane slozky
     * @param is_root - true pokud root slozka, jinak false
     * @return vektor polozek adresare
     */
    std::vector<kiv_os::TDir_Entry>
    Get_Directory_Entries(std::vector<unsigned char> content, const size_t clusters_count, const bool is_root) {
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
                    break;
                }
                dir_entry.file_name[file_name_pos++] = static_cast<char>(content.at(i));
                i++;
            }

            //file_name_pos = j;

            dir_entry.file_name[file_name_pos++] = kCurDirChar;

            for (int j = 0; j < kFileExtensionSize; ++j) {
                if (content.at(i) == kSpaceChar) { // konec pripony souboru
                    i += kFileExtensionSize - j; // dopocteno do 3 bytu
                    break;
                }
                dir_entry.file_name[file_name_pos++] = static_cast<char>(content.at(i));
                i++;
            }


            if (file_name_pos <= kFileNameAndExtensionMaxSize) {
                dir_entry.file_name[file_name_pos] = kEndOfString;
            }
            if (dir_entry.file_name[file_name_pos - 1] == kCurDirChar) { // bez pripony
                dir_entry.file_name[file_name_pos - 1] = kEndOfString;
            }

            dir_entry.file_attributes = content.at(i);

            i += kDirItemAttributesSize; // preskocit atributy
            i += kDirItemUnusedBytes; // preskocit - nedulezite informace
            i += kDirItemClusterBytes; // cislo clusteru
            i += kDirItemFileSizeBytes; // velikost souboru

            dir_content.push_back(dir_entry);
        }

        if (is_root) { // je root - odstranit odkaz na aktualni polozku
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
    kiv_os::NOS_Error Create_File_Or_Dir(Path &path, const uint8_t attributes, std::vector<unsigned char> &fat, const bool is_dir) {
        std::string folder_name = path.full_name;

        path.Delete_Name_From_Path(); // cesta, tzn. bez posledni polozky (jmena)

        std::vector<int> sectors_indexes;
        bool is_root;
        const int start_sector = Get_Start_Sector(path, fat, is_root, sectors_indexes);

        const std::vector<DirItem> directory_items = Get_Folders_From_Dir(fat, start_sector);

        // nalezt 1 volny cluster
        const int free_index = Get_Free_Index(fat);

        if (free_index == -1) { // neni misto
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        } else {
            Write_Value_To_Fat(fat, free_index, kEndClusterInt); // zapise do FAT konec clusteru
            Save_Fat(fat);
        }

        std::vector<unsigned char> buffer_to_write;

        // pridat jmeno souboru
        buffer_to_write.insert(buffer_to_write.end(), path.name.begin(), path.name.end());

        for (int i = static_cast<int>(path.name.size()); i < kFileNameSize; i++) { //doplnit na 8 bytu
            buffer_to_write.push_back(kSpaceChar);
        }

        // pridat priponu - slozka nema zadnou - path_vector.extension.size() je 0

        buffer_to_write.insert(buffer_to_write.end(), path.extension.begin(), path.extension.end());

        for (int i = static_cast<int>(path.extension.size()); i < kFileExtensionSize; i++) { //doplnit na 3 bytu
            buffer_to_write.push_back(kSpaceChar);
        }

        buffer_to_write.push_back(attributes); // atributy

        // nedulezite (cas vytvoreni atd.)
        for (int j = 0; j < kDirItemUnusedBytes; ++j) {
            buffer_to_write.push_back(kSpaceChar);
        }

        // cislo clusteru
        std::vector<unsigned char> bytes_from_cluster_num = Get_Bytes_From_Int(free_index);

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
            extra_dir_item += 1; // navic '.'
            if ((directory_items.size() + extra_dir_item) > (sectors_indexes.size() * kMaxItemsPerCluster)) {
                can_add = false;
            }
        } else { // ne root
            extra_dir_item += 2; // navic '.' a '..'
            if ((directory_items.size() + extra_dir_item) > (sectors_indexes.size() * kMaxItemsPerCluster)) {
	            const int new_cluster_pos = Allocate_New_Cluster(start_sector, fat);
                if (new_cluster_pos == -1) { // nepovedlo se alokovat novy cluster - nelze pridat
                    can_add = false;
                } else {
                    sectors_indexes.push_back(new_cluster_pos);
                }
            }
        }

        if (can_add) {
	        std::vector<char> buffer_to_save;
	        // muzu pridat tak pridam
	        const size_t cluster_pos = (directory_items.size() + extra_dir_item - 1) /
                                 kMaxItemsPerCluster; // poradi clusteru - jeden pojme 16 polozek
	        const size_t item_pos =
                    (directory_items.size() + extra_dir_item - 1) % kMaxItemsPerCluster; // poradi v ramci clusteru

            std::vector<unsigned char> cluster_data = Read_Data_From_Cluster(1, sectors_indexes.at(cluster_pos),
                                                                             is_root);

            for (int j = 0; j < buffer_to_write.size(); ++j) {
                cluster_data.at(item_pos * kDirItemSize + j) = buffer_to_write.at(j);
            }

	        buffer_to_save.reserve(cluster_data.size());
	        for (const unsigned char &data: cluster_data) {
                buffer_to_save.push_back(static_cast<char>(data));
            }
            Write_Data_To_Cluster(buffer_to_save, sectors_indexes.at(cluster_pos), is_root);
        } else { // nelze alokovat cluster
            Write_Value_To_Fat(fat, free_index, 0); // uvolni misto
            Save_Fat(fat);
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        }

        if (is_dir) { // je slozka tak zapsat jeste '.' a '..'
            Write_Current_And_Parent_Folder(free_index, start_sector);
        }

        return kiv_os::NOS_Error::Success;
    }

    /**
     * Zmeni veliksot souboru v polozce adresare
     * @param file_name cesta k souboru - neparsovana
     * @param new_size nova velikost
     * @param fat FAT
     */
    void Change_File_Size(const std::string& file_name, const size_t new_size, const std::vector<unsigned char> &fat) {
        Path path(file_name);
        path.Delete_Name_From_Path();

        std::vector<int> sectors_indexes;
        bool is_root;
        const int start_sector = Get_Start_Sector(path, fat, is_root, sectors_indexes);

        const int item_index = Get_Item_Index(path, start_sector, fat);

        const std::vector<unsigned char> file_size_bytes = Get_Bytes_From_Int_4(static_cast<int>(new_size));

        const size_t cluster_pos = item_index / kMaxItemsPerCluster; // poradi slozky
        const size_t item_pos = item_index % kMaxItemsPerCluster; // poradi v ramci clusteru

        std::vector<unsigned char> data_folder = Read_Data_From_Cluster(1, sectors_indexes.at(cluster_pos),
                                                                        path.path_vector.empty());
        // vlozit novou velikost
        for (int i = kDirItemFileSizePos; i < kDirItemFileSizePos + kDirItemFileSizeBytes; ++i) {
            data_folder.at(item_pos * kDirItemSize + i) = file_size_bytes.at(i - kDirItemFileSizePos);
        }
        std::vector<char> buffer_to_write;
        buffer_to_write.insert(buffer_to_write.end(), data_folder.begin(), data_folder.end());

        Write_Data_To_Cluster(buffer_to_write, sectors_indexes.at(cluster_pos), path.path_vector.empty());
    }

    /**
     * Prevede vektor struktur TDir_Entry na vektor bytu
     * @param entries vektor struktur TDir_Entry
     * @return vektor struktur TDir_Entry prevedeny na vektor bytu
     */
    std::vector<char> Convert_Dir_Entries_To_Char_Vector(std::vector<kiv_os::TDir_Entry> entries) {
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
    kiv_os::NOS_Error
    Get_Or_Set_Attributes(Path path, uint8_t &attributes, const std::vector<unsigned char> &fat, const bool read) {
        path.Delete_Name_From_Path(); // smazat jmeno z cesty

        std::vector<int> sectors_indexes;
        bool is_root;
        const int start_sector = Get_Start_Sector(path, fat, is_root, sectors_indexes); // cislo sektoru

        const int item_index = Get_Item_Index(path, start_sector, fat);

        if (item_index == -1) { // nenalezen
            return kiv_os::NOS_Error::File_Not_Found;
        }


        const size_t cluster_pos = item_index / kMaxItemsPerCluster; // poradi slozky
        const size_t item_pos = item_index % kMaxItemsPerCluster; // poradi v ramci clusteru

        std::vector<unsigned char> folder_data = Read_Data_From_Cluster(1, sectors_indexes.at(cluster_pos), is_root);

        if (read) { // cteme
            attributes = folder_data.at(item_pos * kDirItemSize + kDirItemAttributesPos);
        } else { // zapisujeme
            folder_data.at(item_pos * kDirItemSize + kDirItemAttributesPos) = attributes; // zapsat atributy

            std::vector<char> buffer_to_write;
            buffer_to_write.insert(buffer_to_write.end(), folder_data.begin(), folder_data.end());
            Write_Data_To_Cluster(buffer_to_write, sectors_indexes.at(cluster_pos), is_root);
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
    int Get_Start_Sector(const Path &path, const std::vector<unsigned char> &fat, bool &is_root,
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
            const DirItem dir_item = Get_Dir_Item_Cluster(kRootDirSectorStart, path, fat);
            sectors_indexes = Get_Sectors_Indexes(fat, dir_item.first_cluster);
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
    int Get_Item_Index(const Path &path, const int start_sector, const std::vector<unsigned char> &fat) {
	    const std::vector<DirItem> directory_items = Get_Folders_From_Dir(fat, start_sector);

        int item_index = -1;
        for (int i = 0; i < directory_items.size(); ++i) {
            // kontrola, jestli ma stejny nazev i priponu
            if (Check_Name_Matches(path, directory_items.at(i))) {
                item_index = i;
                break;
            }
        }

        if (item_index == -1) { // nenalezen
            return -1;
        }

        if (path.path_vector.empty()) {
            item_index += 1; // pridat '.'
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
    bool Check_Name_Matches(const Path &path, const DirItem &dir_item) {
        if (dir_item.extension == path.extension && dir_item.file_name == path.name) {
            return true;
        }
        return false;
    }
}