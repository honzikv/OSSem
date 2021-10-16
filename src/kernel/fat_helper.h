//
// Created by Kuba on 10.10.2021.
//

#ifndef OS_FAT_HELPER_H
#define OS_FAT_HELPER_H

#include "path.h"

const int FAT_TABLE_SECTOR_COUNT = 9;
const int SECTOR_SIZE = 512;
const int DISK_NUM = 129;
const int ROOT_DIR_SECTOR_START = 19;
const int USER_DATA_START = 33;
const int ROOT_DIR_SIZE = 14;
const int RESERVED_ENTRIES = 2; // pozice 0 a 1 jsou rezervovane
const int DATA_SECTOR_CONVERSION = USER_DATA_START - RESERVED_ENTRIES; //data sector zacina na pozici 33, ale prvni cluster je vzdy 2 (33 - 2 = 31)
const int FAT_ADDRESS_SIZE = 12;
const int BITS_IN_BYTES = 8;
const int BITS_IN_BYTES_HALVED = (int) (BITS_IN_BYTES * 0.5);
const double INDEX_TO_FAT_CONVERSION = (double) FAT_ADDRESS_SIZE / BITS_IN_BYTES; // ration na prepocet pozice do indexu v FAT
const int MAX_CLUSTER_NUM = 4096;
const int FILE_NAME_SIZE = 8; // maximalni delka nazvu slozky/souboru
const int FILE_EXTENSION_SIZE = 3; // maximalni delka pripony slozky/souboru
const int DIR_ITEM_ATTR_TO_CLUSTER = 14; // pocet bytu mezi atributy a cislem prvniho clusteru v zaznamu
const int DIR_ITEM_CLUSTER_BYTES = 2; // pocet bytu udavajici cislo clusteru
const int DIR_ITEM_FILE_SIZE_BYTES = 4; // pocet bytu udavajici velikost souboru
const int END_CLUSTER_INT = 4095; // oznacuje konec v clusteru
const int MAX_ITEMS_CLUSTER = 16;

struct DirItem {
    std::string fileName;
    std::string fileExtension;
    size_t fileSize;
    int firstCluster;
    unsigned char attribute;
};

std::vector<unsigned char> loadFatTable(int startIndex);

std::vector<unsigned char> readFromRegisters(int clusterCount, int sectorSize, int startIndex);

uint16_t getFreeIndex(std::vector<unsigned char> fat);

std::vector<int> getSectorsIndexes(const std::vector<unsigned char>& fat, int start);

DirItem getDirItemCluster(int startCluster, const Path& path, const std::vector<unsigned char >& fat);

std::vector<DirItem> getFoldersFromDir (const std::vector<unsigned char>& fat, int sectorNum);

void writeValueToFat(std::vector<unsigned char> &fat, int pos, int newValue);

void writeToRegisters(std::vector<char> buffer, int startIndex);

void saveFat(const std::vector<unsigned char >& fat);

std::vector<unsigned char > getBytesFromInt(int value);

int allocateNewCluster(int startCluster, std::vector<unsigned char > &fat);




#endif //OS_FAT_HELPER_H
