//
// Created by Kuba on 10.10.2021.
//

#pragma once

#include "../../api/api.h"
#include "path.h"
namespace Fat_Helper {

    const int kFatTableSectorCount = 9; // pocet sektoru vyhrazenych pro FAT
    const int kSectorSize = 512; // velikost sektoru/clusteru
    const int kDiskNum = 129; // cislo disku
    const int kRootDirSectorStart = 19; // pozice sektoru rootu
    const int kUserDataStart = 33; // pozice sektoru s daty
    const int kRootDirSize = 14; // velikost rootu ípocet sektorué
    const int kReservedEntries = 2; // pozice 0 a 1 jsou rezervovane
    const int kDataSectorConversion =
            kUserDataStart -
            kReservedEntries; //data sector zacina na pozici 33, ale prvni cluster je vzdy 2 (33 - 2 = 31)
    const int kFatAddressSize = 12; // pocet bitu udavajici cislo clusteru v FAT
    const int kBitsInBytes = 8; // pocet bitu v bytu
    const int kBitsInBytesHalved = (int) (kBitsInBytes * 0.5); // pocet bitu v bytu deleno dvema
    const double kIndexToFatConversion =
            (double) kFatAddressSize / kBitsInBytes; // ration na prepocet pozice do indexu v FAT
    const int kMaxClusterIndex = 4096; // maximalni cislo clusteru
    const int kMaxNonEndClusterIndex = 4088; // maximalni cislo clusteru, ktere cislo dalsiho clusteru (ne konec)
    const int kFileNameSize = 8; // maximalni delka nazvu slozky/souboru
    const int kFileExtensionSize = 3; // maximalni delka pripony slozky/souboru
    const int kFileNameAndExtensionMaxSize =
            kFileNameSize + kFileExtensionSize; // maximalni delka celeho nazvu (nazev + pripona)
    const int kDirItemAttributesSize = 1; // pocet bytu atributu v polozce adresare
    const int kDirItemUnusedBytes = 14; // pocet bytu mezi atributy a cislem prvniho clusteru v zaznamu
    const int kDirItemClusterBytes = 2; // pocet bytu udavajici cislo clusteru
    const int kDirItemClusterPos = 26; // pocatecni pozice bytu udavajici cislo clusteru
    const int kDirItemFileSizeBytes = 4; // pocet bytu udavajici velikost souboru
    const int kDirItemFileSizePos = 28; // pocatecni pozice bytu udavajici velikost souboru
    const int kEndClusterInt = 4095; // oznacuje konec v clusteru
    const int kMaxItemsPerCluster = 16; // maximalni pocet polozek adresare v clusteru
    const int kDirItemSize = 32; // velikost v bytech polozky adresare
    const int kDirItemAttributesPos = 11; // pozice atributu v polozce adresare
    const char kCurDirChar = '.'; // odkaz na aktualni adresar
    const char kSpaceChar = ' '; // mezera - zapisuje se napr. pri kratsim jmene nez 8 znaku
    const char kEndOfString = '\0'; // znak oznacujici konec stringu
    const int kFreeDirItem = 246; // oznacuje volnou polozku adresare

    struct DirItem {
        std::string file_name;
        std::string extension;
        size_t file_size;
        int first_cluster;
        unsigned char attributes;
    };

    std::vector<unsigned char> Load_Fat_Table(int start_index);

    bool Check_Fat_Consistency(std::vector<unsigned char> first_table, std::vector<unsigned char> second_table);

    std::vector<unsigned char> Read_From_Registers(int cluster_count, int start_index);

    std::vector<unsigned char> Read_Data_From_Cluster(int cluster_count, int start_cluster, bool is_root);

    int Get_Int_From_Char_Vector(const std::vector<unsigned char>& bytes);

    uint16_t Get_Free_Index(const std::vector<unsigned char>& fat);

    std::vector<int> Get_Sectors_Indexes(const std::vector<unsigned char> &fat, int start);

    DirItem Get_Dir_Item_Cluster(int start_cluster, const Path &path, const std::vector<unsigned char> &fat);

    std::vector<DirItem> Get_Folders_From_Dir(const std::vector<unsigned char> &fat, int start_sector);

    void Write_Value_To_Fat(std::vector<unsigned char> &fat, int pos, int new_value);

    void Write_To_Registers(std::vector<char> buffer, int start_index);

    void Write_Data_To_Cluster(std::vector<char> buffer, int start_cluster, bool is_root);

    void Save_Fat(const std::vector<unsigned char> &fat);

    std::vector<unsigned char> Get_Bytes_From_Int(int value);

    int Allocate_New_Cluster(int start_cluster, std::vector<unsigned char> &fat);

    bool Validate_File_Name(const std::string &file_name);

    std::vector<kiv_os::TDir_Entry> Read_Directory(const Path& path, const std::vector<unsigned char> &fat);

    std::vector<kiv_os::TDir_Entry>
    Get_Directory_Entries(std::vector<unsigned char> content, size_t clusters_count, bool is_root);

    kiv_os::NOS_Error Create_File_Or_Dir(Path &path, uint8_t attributes, std::vector<unsigned char> &fat, bool is_dir);

    void Change_File_Size(const std::string& file_name, size_t new_size, const std::vector<unsigned char> &fat);

    std::vector<char> Convert_Dir_Entries_To_Char_Vector(std::vector<kiv_os::TDir_Entry> entries);

    kiv_os::NOS_Error
    Get_Or_Set_Attributes(Path path, uint8_t &attributes, const std::vector<unsigned char> &fat, bool read);

    int Get_Start_Sector(const Path &path, const std::vector<unsigned char> &fat, bool &is_root,
                         std::vector<int> &sectors_indexes);

    int Get_Item_Index(const Path &path, int start_sector, const std::vector<unsigned char> &fat);

    bool Check_Name_Matches(const Path &path, const DirItem &dir_item);

}
