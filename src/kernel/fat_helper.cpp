//
// Created by Kuba on 10.10.2021.
//

#include <vector>
#include <hal.h>
#include <string>
#include "fat_helper.h"
#include "path.h"
#include "api.h"

/**
 * Nacte FAT tabulku na specifickem indexu (urcuje 1. / 2. tabulku)
 * @param startIndex index zacatku tabulky
 * @return FAT tabulka
 */
std::vector<unsigned char> loadFatTable(int startIndex) {
    std::vector<unsigned char> table;

    table = readFromRegisters(FAT_TABLE_SECTOR_COUNT, SECTOR_SIZE, startIndex);
    return table;
}

/**
 * Zkontroluje, zdali je obsah obou tabulek FAT totozny
 * @param firstTable prvni FAT tabulka
 * @param secondTable druha FAT tabulka
 * @return true pokud jsou FAT tabulky totozne, jinak false
 */
bool checkFatConsistency(std::vector<unsigned char> firstTable, std::vector<unsigned char> secondTable) {
    for (int i = 0; i < static_cast<int>(firstTable.size()); ++i) {
        if (firstTable[i] != secondTable[i]) {
            return false;
        }
    }
    return true;
}

/**
 * Nacte obsah root directory nachazejici se v sektorech 19 az 32. Kazde zaznam v adresari obsahuje napr. jmeno souboru a cislo prvnoho clusteru
 * @return vektor zaznamu v adresari
 */
std::vector<unsigned char> loadRootDirectory() {
    std::vector<unsigned char> rootDir;

    rootDir = readFromRegisters(ROOT_DIR_SIZE, SECTOR_SIZE, ROOT_DIR_SECTOR_START);

    return rootDir;
}

/**
 * Precte data zacinajici a koncici na danych clusterech (tzn. sektorech)
 * @param startCluster cislo prvniho clusteru
 * @param clusterCount pocet clusteru k precteni
 * @return data z clusteru
 */
std::vector<unsigned char> readDataFromCluster(int startCluster, int clusterCount) {
    std::vector<unsigned char> bytes;
    int startIndex = startCluster + DATA_SECTOR_CONVERSION;

    bytes = readFromRegisters(clusterCount, SECTOR_SIZE, startIndex);

    return bytes;
}

//TODO komentar
/**
 * Nacte data z registru
 * @param clusterCount
 * @param sectorSize
 * @param startIndex
 * @return
 */
std::vector<unsigned char> readFromRegisters(int clusterCount, int sectorSize, int startIndex) {
    std::vector<unsigned char> result;

    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet addressPacket;

    int size = clusterCount * sectorSize;

    auto sector = new std::string[size]; //TODO size
    addressPacket.count = clusterCount;
    addressPacket.sectors = (void *) sector;
    addressPacket.lba_index = startIndex;

    registers.rdx.l = DISK_NUM;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&addressPacket);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers); //TODO error

    char *buffer = reinterpret_cast<char *>(addressPacket.sectors);

    for (int i = 0; i < size; ++i) {
        result.push_back(buffer[i]);
    }
    return result;
}

//TODO koment
void writeToRegisters(std::vector<char> buffer, int startIndex) {
    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet addressPacket;

    addressPacket.count = buffer.size() / SECTOR_SIZE + (buffer.size() % SECTOR_SIZE);
    addressPacket.lba_index = startIndex + DATA_SECTOR_CONVERSION;

    registers.rdx.l = DISK_NUM;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&addressPacket);


    // cast posledniho sektoru nemusi byt vzdy prepsana - mohou tam byt jina data - nechat
    int lastSector = static_cast<int>(addressPacket.count) + startIndex - 1;
    std::vector<unsigned char> lastSectorData = readFromRegisters(1, SECTOR_SIZE, lastSector);
    int keep = static_cast<int>(addressPacket.count) * SECTOR_SIZE;

    int lastTaken = buffer.size() % SECTOR_SIZE;

    size_t start = buffer.size();
    int bytesAdded = 0;
    for (size_t i = start; i < keep; ++i) {
        buffer.push_back(lastSectorData.at(static_cast<size_t>(lastTaken) + static_cast<size_t>(bytesAdded)));
        bytesAdded++;
    }

    addressPacket.sectors = static_cast<void*>(buffer.data());

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers); //TODO error
}

/**
 * Ziska cislo clusteru z FAT
 * @param fat FAT
 * @param pos pozice do FAT
 * @return cislo clusteru
 */
uint16_t getClusterNum(std::vector<unsigned char> fat, int pos) {
    int index = (int) (pos * INDEX_TO_FAT_CONVERSION);
    uint16_t clusterNum = 0;
    if (pos % 2 == 0) { // prvni byte cely + prvni pulka druheho bytu
        clusterNum |= (uint16_t) fat.at(index) << BITS_IN_BYTES_HALVED;
        clusterNum |= ((uint16_t) fat.at(index + 1) & 0xF0) >> BITS_IN_BYTES_HALVED;
    } else { // druha pulka prvniho bytu + cely druhy byte
        clusterNum |= ((uint16_t) fat.at(index) & 0x0F) << BITS_IN_BYTES;
        clusterNum |= (uint16_t) fat.at(index + 1);
    }
    return clusterNum;
}

/**
 * Vypocte a vrati int hodnotu z vektoru bytu
 * @param bytes vektor bytu
 * @return int hodnota z vektoru bytu
 */
int getIntFromCharVector(std::vector<unsigned char> bytes) {
    int res = 0;
    for (int i = 0; i < bytes.size(); i++) {
        res |= (int) bytes.at(i) << ((bytes.size() - 1 - i) * BITS_IN_BYTES);
    }
    return res;
}

/**
 * Vrati vektor bytu z celociselne hodnoty
 * @param value hodnota
 * @return vektor bytu
 */
std::vector<unsigned char > getBytesFromInt(int value) {
    std::vector<unsigned char> res;
    res.push_back(value >> 8);
    res.push_back(value & 0xFF);
    return res;
}


/**
 * Najde a vrati ve FAT index prvniho volneho clusteru
 * @param fat FAT
 * @return index prvniho volneho clusteru, -1 pokud nenalezen
 */
uint16_t getFreeIndex(std::vector<unsigned char> fat) {
    for (int i = 0; i < fat.size(); i += 2) {
        uint16_t clusterNum = 0;
        clusterNum |= (uint16_t) fat.at(i) << BITS_IN_BYTES_HALVED;
        clusterNum |= ((uint16_t) fat.at(i + 1) & 0xF0) >> BITS_IN_BYTES_HALVED;
        if (clusterNum == 0) {
            return i;
        }
        clusterNum |= ((uint16_t) fat.at(i + 1) & 0x0F) << BITS_IN_BYTES;
        clusterNum |= (uint16_t) fat.at(i + 2);
        if (clusterNum == 0) {
            return i + 1;
        }
    }
    return -1;
}

/**
 * Ulozi do FAT na danou pozici novou hodnotu
 * @param fat FAT
 * @param pos pozice
 * @param newValue nova hodnota
 */
void writeValueToFat(std::vector<unsigned char> &fat, int pos, int newValue) {
    int index = (int) (pos * INDEX_TO_FAT_CONVERSION);
    if (pos % 2 == 0) {
        fat.at(index) = newValue >> BITS_IN_BYTES_HALVED;
        int pom = (uint16_t) fat.at(index + 1) & 0x0F | (newValue & 0x0F) << BITS_IN_BYTES_HALVED;
        fat.at(index + 1) = pom;
    } else {
        int pom = (uint16_t) fat.at(index) & 0xF0 | (newValue & 0xF00) >> BITS_IN_BYTES;
        fat.at(index) = pom;
        fat.at(index + 1) = newValue & 0xFF;
    }
}

/**
 * Ulozi FAT
 * @param fat FAT
 */
void saveFat(const std::vector<unsigned char >& fat) {
    std::vector<char> fatChar;
    for (unsigned char i : fat) {
        fatChar.push_back(i);
    }
    writeToRegisters(fatChar, 1 - DATA_SECTOR_CONVERSION);
    writeToRegisters(fatChar, 1 + FAT_TABLE_SECTOR_COUNT - DATA_SECTOR_CONVERSION);
}


/**
 * Ziska a vrati seznam sektoru obsahujici dany soubor
 * @param fat FAT
 * @param start prvni sektor souboru
 * @return seznam sektoru daneho souboru
 */
std::vector<int> getSectorsIndexes(const std::vector<unsigned char> &fat, int start) {
    std::vector<int> sectors;

    sectors.push_back(start);
    start = getClusterNum(fat, start);

    while (start < MAX_CLUSTER_NUM) { //TODO mozna 4088 a to push prvni taky mozna check
        sectors.push_back(start);
        start = getClusterNum(fat, start);
    }
    return sectors;
}


/**
 * Ziska a vrati vektor polozek adresare (obsah slozky)
 * @param content vektor s byty jedne slozky
 * @param sectorsCount pocet sektoru dane slozky
 * @return vektor polozek adresare
 */
std::vector<DirItem> getDirectoryItems(std::vector<unsigned char> content, int sectorsCount) {
    std::vector<DirItem> dirContent;

    for (int i = 0; i < SECTOR_SIZE * sectorsCount; ++i) {
        if (content[i] == 0 || content[i] == 246) { //TODO check a konstanta
            break;
        }

        DirItem dirItem;
        dirItem.fileName = "";

        for (int j = 0; j < FILE_NAME_SIZE; ++j) {
            if (content[i] == ' ') { //konec nazvu souboru //TODO konstanta
                i += FILE_NAME_SIZE - j; //dopocteno do 8 bytu
                break;
            } else {
                dirItem.fileName.push_back(content[i]);
                i++;
            }
        }

        dirItem.fileExtension = "";
        for (int j = 0; j < FILE_EXTENSION_SIZE; ++j) {
            if (content[i] == ' ') { //konec nazvu souboru //TODO konstanta
                i += FILE_NAME_SIZE - j; //dopocteno do 8 bytu
                break;
            } else {
                dirItem.fileName.push_back(content[i]);
                i++;
            }
        }

        dirItem.attribute = content[i++];

        i += DIR_ITEM_ATTR_TO_CLUSTER;

        std::vector<unsigned char> clusterBytes;

        for (int j = 0; j < DIR_ITEM_CLUSTER_BYTES; ++j) {
            clusterBytes.push_back(content[i++]);
        }

        dirItem.firstCluster = getIntFromCharVector(clusterBytes);


        std::vector<unsigned char> fileSizeBytes;

        for (int j = 0; j < DIR_ITEM_FILE_SIZE_BYTES; ++j) {
            fileSizeBytes.push_back(content[i++]);
        }

        dirItem.firstCluster = getIntFromCharVector(fileSizeBytes);


        dirContent.push_back(dirItem);

    }
    return dirContent;
}

/**
 * Ziska a vrati obsah dane slozky
 * @param fat FAT
 * @param sectorNum cislo prvniho sektoru dane slozky
 * @return vektor polozek adresare v dane slozce
 */
std::vector<DirItem> getFoldersFromDir(const std::vector<unsigned char> &fat, int sectorNum) {
    if (sectorNum == ROOT_DIR_SECTOR_START) {
        std::vector<unsigned char> dataClusters = readDataFromCluster(ROOT_DIR_SECTOR_START - DATA_SECTOR_CONVERSION,
                                                                      ROOT_DIR_SIZE);

        std::vector<DirItem> content = getDirectoryItems(dataClusters, ROOT_DIR_SIZE); //TODO udelat

        content.erase(content.begin());

        return content;

    } else {
        std::vector<int> dataSectors = getSectorsIndexes(fat, sectorNum);

        std::vector<unsigned char> dataClusters;

        std::vector<DirItem> dirItems;

        for (int i = 0; i < dataSectors.size(); ++i) {
            dataClusters = readDataFromCluster(dataSectors[i], 1); // obsah jednoho clusteru
            std::vector<DirItem> content = getDirectoryItems(dataClusters, 1);

            int j = i == 0 ? 2 : 0; // prvni cluster slozky - obsahuje i '.' a '..' - preskocit

            for (; j < content.size(); j++) {
                dirItems.push_back(content.at(j));
            }

        }
        return dirItems;
    }
}

/**
 * Vrati polozku adresare, na kterem zacina hledany soubor/slozka
 * @param startCluster pocatecni cluster, kde se hleda soubor/slozka
 * @param path cesta k souboru/slozce
 * @param fat FAT
 * @return polozka adresare
 */
DirItem getDirItemCluster(int startCluster, const Path &path, const std::vector<unsigned char> &fat) {
    if (path.path.empty()) { // root
        DirItem dirItem;
        dirItem.firstCluster = ROOT_DIR_SECTOR_START;
        dirItem.fileName = "/";
        dirItem.fileExtension = "";
        dirItem.fileSize = 0;
        dirItem.attribute = static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID);
    }

    std::vector<DirItem> curFolderItems; // polozky v prave prochazene slozce
    int directoryItemNum;
    for (auto &i: path.path) {
        curFolderItems = getFoldersFromDir(fat, startCluster);

        directoryItemNum = -1;

        for (int j = 0; j < curFolderItems.size(); ++j) {
            DirItem dirItem = curFolderItems.at(j);
            std::string dirItemFullName = dirItem.fileName;
            if (!dirItem.fileExtension.empty()) {
                dirItemFullName += "." + dirItem.fileExtension; //TODO mozna konstanta '.'
            }
            if (i == dirItemFullName) {
                directoryItemNum = j;
                break;
            }
        }
        if (directoryItemNum == -1) {
            break;
        }

        DirItem dirItem = curFolderItems.at(directoryItemNum);

        startCluster = dirItem.firstCluster;

        if (dirItem.firstCluster == 0) {
            dirItem.firstCluster = ROOT_DIR_SECTOR_START;
        }

    }

    DirItem dirItem = {};
    if (directoryItemNum == -1) {
        dirItem.firstCluster = -1;
        dirItem.fileSize = -1;
    } else {
        dirItem = curFolderItems.at(directoryItemNum);
        if (dirItem.firstCluster == 0) {
            dirItem.firstCluster = ROOT_DIR_SECTOR_START;
        }
    }
    return dirItem; //TODO neco s adresou
}

/**
 * Alokuje novy cluster souboru/slozce zacinajici na dane pozici
 * @param startCluster prvni cluster
 * @param fat FAT
 * @return cislo clusteru / -1, pokud se nepodarilo
 */
int allocateNewCluster(int startCluster, std::vector<unsigned char > &fat) {
    int freeIndex = getFreeIndex(fat);
    if (freeIndex == -1) {
        return -1;
    } else{
        std::vector<int> sectorsIndexes = getSectorsIndexes(fat, startCluster);

        writeValueToFat(fat, freeIndex, END_CLUSTER_INT); // zapise do FAT konec clusteru
        writeValueToFat(fat, sectorsIndexes.back(), freeIndex); // zapise do FAT konec clusteru


        saveFat(fat);

        return freeIndex;
    }
}
