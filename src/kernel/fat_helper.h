//
// Created by Kuba on 10.10.2021.
//

#ifndef OS_FAT_HELPER_H
#define OS_FAT_HELPER_H

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


#endif //OS_FAT_HELPER_H
