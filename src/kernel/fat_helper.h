//
// Created by Kuba on 10.10.2021.
//

#ifndef OS_FAT_HELPER_H
#define OS_FAT_HELPER_H

#include <api.h>
#include "path.h"

const int kFatTableSectorCount = 9;
const int kSectorSize = 512;
const int kDiskNum = 129;
const int kRootDirSectorStart = 19;
const int kUserDataStart = 33;
const int kRootDirSize = 14;
const int kReservedEntries = 2; // pozice 0 a 1 jsou rezervovane
const int kDataSectorConversion =
        kUserDataStart - kReservedEntries; //data sector zacina na pozici 33, ale prvni cluster je vzdy 2 (33 - 2 = 31)
const int kFatAddressSize = 12;
const int kBitsInBytes = 8;
const int kBitsInBytesHalved = (int) (kBitsInBytes * 0.5);
const double kIndexToFatConversion =
        (double) kFatAddressSize / kBitsInBytes; // ration na prepocet pozice do indexu v FAT
const int kMaxClusterNum = 4096;
const int kFileNameSize = 8; // maximalni delka nazvu slozky/souboru
const int kFileExtensionSize = 3; // maximalni delka pripony slozky/souboru
const int kDirItemUnusedBytes = 14; // pocet bytu mezi atributy a cislem prvniho clusteru v zaznamu
const int kDirItemClusterBytes = 2; // pocet bytu udavajici cislo clusteru
const int kDirItemFileSizeBytes = 4; // pocet bytu udavajici velikost souboru
const int kEndClusterInt = 4095; // oznacuje konec v clusteru
const int kMaxItemsPerCluster = 16; // maximalni pocet polozek v clusteru
const int kDirItemSize = 32; // veliksot v bytech polozky

struct DirItem {
    std::string file_name;
    std::string file_extension;
    size_t file_size;
    int first_cluster;
    unsigned char attribute;
};

std::vector<unsigned char> LoadFatTable(int start_index);

std::vector<unsigned char> ReadFromRegisters(int cluster_count, int sector_size, int start_index);

uint16_t GetFreeIndex(std::vector<unsigned char> fat);

std::vector<int> GetSectorsIndexes(const std::vector<unsigned char> &fat, int start);

DirItem GetDirItemCluster(int start_cluster, const Path &path, const std::vector<unsigned char> &fat);

std::vector<DirItem> GetFoldersFromDir(const std::vector<unsigned char> &fat, int sector_num);

void WriteValueToFat(std::vector<unsigned char> &fat, int pos, int new_value);

void WriteToRegisters(std::vector<char> buffer, int start_index);

void SaveFat(const std::vector<unsigned char> &fat);

std::vector<unsigned char> GetBytesFromInt(int value);

int AllocateNewCluster(int start_cluster, std::vector<unsigned char> &fat);

bool ValidateFileName(const std::string &file_name);

std::vector<kiv_os::TDir_Entry> ReadDirectory(Path path, const std::vector<unsigned char> &fat);

std::vector<kiv_os::TDir_Entry>
GetDirectoryEntries(std::vector<unsigned char> content, size_t clusters_count, bool is_root);

kiv_os::NOS_Error CreateFileOrDir(Path &path, uint8_t attributes, std::vector<unsigned char> &fat, bool is_dir);

#endif //OS_FAT_HELPER_H
