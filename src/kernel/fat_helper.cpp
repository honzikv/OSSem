//
// Created by Kuba on 10.10.2021.
//

#include <vector>
#include <hal.h>
#include <string>
#include "fat_helper.h"

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
