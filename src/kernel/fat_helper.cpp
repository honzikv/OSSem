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
    int sectorSize = SECTOR_SIZE * FAT_TABLE_SECTOR_COUNT;
    std::vector<unsigned char> table;

    table = readFromRegisters(FAT_TABLE_SECTOR_COUNT, sectorSize, startIndex);
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
    int sectorSize = SECTOR_SIZE * ROOT_DIR_SIZE;

    rootDir = readFromRegisters(ROOT_DIR_SIZE, sectorSize, ROOT_DIR_SECTOR_START);

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
    int sectorSize = SECTOR_SIZE * clusterCount;
    int startIndex = startCluster + DATA_SECTOR_CONVERSION;

    bytes = readFromRegisters(clusterCount, sectorSize, startIndex);

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

    auto sector = new std::string[sectorSize];
    addressPacket.count = clusterCount;
    addressPacket.sectors = (void *) sector;
    addressPacket.lba_index = startIndex;

    registers.rdx.l = DISK_NUM;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&addressPacket);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers); //TODO error

    char *buffer = reinterpret_cast<char *>(addressPacket.sectors);

    for (int i = 0; i < sectorSize; ++i) {
        result.push_back(buffer[i]);
    }
    return result;
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
 * Najde a vrati ve FAT index prvniho volneho clusteru
 * @param fat FAT
 * @return index prvniho volneho clusteru, -1 pokud nenalezen
 */
uint16_t getFreeIndex(std::vector<unsigned char> fat) {
    for (int i = 0; i < fat.size(); i+=2) {
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
 * Ziska a vrati seznam sektoru obsahujici dany soubor
 * @param fat FAT
 * @param start prvni sektor souboru
 * @return seznam sektoru daneho souboru
 */
std::vector<int> getSectorsIndexes(const std::vector<unsigned char>& fat, int start) {
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
 * Ziska a vrati obsah dane slozky
 * @param fat FAT
 * @param sectorNum cislo prvniho sektoru dane slozky
 * @return vektor polozek adresare v dane slozce
 */
std::vector<DirItem> getFoldersFromDir (const std::vector<unsigned char>& fat, int sectorNum) {
    if (sectorNum == ROOT_DIR_SECTOR_START) {
        std::vector<unsigned char> dataClusters = readDataFromCluster(ROOT_DIR_SECTOR_START - DATA_SECTOR_CONVERSION,
                                                                      ROOT_DIR_SIZE);

        std::vector<DirItem> content = getDirectoryItems(); //TODO udelat

        content.erase(content.begin());

        return content;

    } else {
        std::vector<int> dataSectors = getSectorsIndexes(fat, sectorNum);

        std::vector<unsigned char> dataClusters;

        std::vector<DirItem> dirItems;

        for (int i = 0; i < dataSectors.size(); ++i) {
            dataClusters = readDataFromCluster(dataSectors[i], 1); // obsah jednoho clusteru
            std::vector<DirItem> content = getDirectoryItems();

            int j = i == 0 ? 2 :0; // prvni cluster slozky - obsahuje i '.' a '..' - preskocit

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
DirItem getDirItemCluster(int startCluster, const Path& path, const std::vector<unsigned char >& fat) {
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
    for (auto & i : path.path) {
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
