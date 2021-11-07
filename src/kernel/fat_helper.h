//
// Created by Kuba on 10.10.2021.
//

#ifndef OS_FAT_HELPER_H
#define OS_FAT_HELPER_H

#include "../api/api.h"
#include "path.h"
//TODO comment
const int kFatTableSectorCount = 9; // pocet sektoru vyhrazenych pro FAT
const int kSectorSize = 512; // velikost sektoru/clusteru
const int kDiskNum = 129;
const int kRootDirSectorStart = 19; // pozice sektoru rootu
const int kUserDataStart = 33; // pozice sektoru s daty
const int kRootDirSize = 14; // velikost rootu ípocet sektorué
const int kReservedEntries = 2; // pozice 0 a 1 jsou rezervovane
const int kDataSectorConversion =
        kUserDataStart - kReservedEntries; //data sector zacina na pozici 33, ale prvni cluster je vzdy 2 (33 - 2 = 31)
const int kFatAddressSize = 12; // pocet bitu udavajici cislo clusteru v FAT
const int kBitsInBytes = 8; // pocet bitu v bytu
const int kBitsInBytesHalved = (int) (kBitsInBytes * 0.5); // pocet bitu v bytu deleno dvema
const double kIndexToFatConversion =
        (double) kFatAddressSize / kBitsInBytes; // ration na prepocet pozice do indexu v FAT
const int kMaxClusterIndex = 4096; // maximalni cislo clusteru
const int kMaxNonEndClusterIndex = 4088; // maximalni cislo clusteru, ktere cislo dalsiho clusteru (ne konec)
const int kFileNameSize = 8; // maximalni delka nazvu slozky/souboru
const int kFileExtensionSize = 3; // maximalni delka pripony slozky/souboru
const int kFileNameAndExtensionMaxSize = kFileNameSize + kFileExtensionSize; // maximalni delka celeho nazvu (nazev + pripona)
const int kDirItemAttributesSize = 1; // pocet bytu atributu v polozce adresare
const int kDirItemUnusedBytes = 14; // pocet bytu mezi atributy a cislem prvniho clusteru v zaznamu
const int kDirItemClusterBytes = 2; // pocet bytu udavajici cislo clusteru
const int kDirItemFileSizeBytes = 4; // pocet bytu udavajici velikost souboru
const int kDirItemFileSizePos = 28; // pocatecni pozice bytu udavajici velikost souboru
const int kEndClusterInt = 4095; // oznacuje konec v clusteru
const int kMaxItemsPerCluster = 16; // maximalni pocet polozek adresare v clusteru
const int kDirItemSize = 32; // velikost v bytech polozky adresare
const int kDirItemAttributesPos = 11; // pozice atributu v polozce adresare
const char kCurDirChar = '.'; // odkaz na aktualni adresar
const char kSpaceChar = ' '; // mezera - zapisuje se napr. pri kratsim jmene nez 8 znaku
const char kEndOfString = '\0'; // znak oznacujici konec stringu
const int kFreeDirItem = 229; // oznacuje volnou polozku adresare

struct DirItem {
    std::string file_name;
    std::string extension;
    size_t file_size;
    int first_cluster;
    unsigned char attributes;
};

std::vector<unsigned char> LoadFatTable(int start_index);

bool CheckFatConsistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table);

std::vector<unsigned char> ReadFromRegisters(int cluster_count, int start_index);

std::vector<unsigned char> ReadDataFromCluster(int cluster_count, int start_index, bool is_root);

uint16_t GetFreeIndex(std::vector<unsigned char> fat);

std::vector<int> GetSectorsIndexes(const std::vector<unsigned char> &fat, int start);

DirItem GetDirItemCluster(int start_cluster, const Path &path, const std::vector<unsigned char> &fat);

std::vector<DirItem> GetFoldersFromDir(const std::vector<unsigned char> &fat, int start_sector);

void WriteValueToFat(std::vector<unsigned char> &fat, int pos, int new_value);

void WriteToRegisters(std::vector<char> buffer, int start_index);

void WriteDataToCluster(std::vector<char> buffer, int start_cluster, bool is_root);

void SaveFat(const std::vector<unsigned char> &fat);

std::vector<unsigned char> GetBytesFromInt(int value);

int AllocateNewCluster(int start_cluster, std::vector<unsigned char> &fat);

bool ValidateFileName(const std::string &file_name);

std::vector<kiv_os::TDir_Entry> ReadDirectory(Path path, const std::vector<unsigned char> &fat);

std::vector<kiv_os::TDir_Entry>
GetDirectoryEntries(std::vector<unsigned char> content, size_t clusters_count, bool is_root);

kiv_os::NOS_Error CreateFileOrDir(Path &path, uint8_t attributes, std::vector<unsigned char> &fat, bool is_dir);

void ChangeFileSize(const char* file_name, size_t new_size, const std::vector<unsigned char>& fat);

std::vector<char> ConvertDirEntriesToChar(std::vector<kiv_os::TDir_Entry> entries);

kiv_os::NOS_Error GetOrSetAttributes(Path path, uint8_t &attributes, const std::vector<unsigned char> &fat, bool read);

int GetStartSector(const Path& path, const std::vector<unsigned char> &fat, bool &is_root, std::vector<int> &sectors_indexes);

int GetItemIndex(const Path &path, int start_sector, const std::vector<unsigned char> &fat);

bool NameMatches(const Path &path, const DirItem& dir_item);

#endif //OS_FAT_HELPER_H
