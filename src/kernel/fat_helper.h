//
// Created by Kuba on 10.10.2021.
//

#ifndef OS_FAT_HELPER_H
#define OS_FAT_HELPER_H

const int FAT_TABLE_SECTOR_COUNT = 9;
const int SECTOR_SIZE = 512;
const int DISK_NUM = 129;
const int ROOT_DIR_SECTOR_START = 19;
const int ROOT_DIR_SIZE = 14;
const int DATA_SECTOR_CONVERSION = 31; //data sector zacina na pozici 33, ale prvni cluster je vzdy 2 (33 - 2 = 31)


std::vector<unsigned char> loadFatTable(int startIndex);

std::vector<unsigned char> readFromRegisters(int clusterCount, int sectorSize, int startIndex);


#endif //OS_FAT_HELPER_H
