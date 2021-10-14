//
// Created by Kuba on 09.10.2021.
//

#include "fat12.h"
#include "fat_helper.h"

std::vector<unsigned char> fat;
std::vector<unsigned char> secondFat;


Fat12::Fat12() {
    fat = loadFatTable(1);
    secondFat = loadFatTable(1 + FAT_TABLE_SECTOR_COUNT);
}

//TODO dodelat vse

kiv_os::NOS_Error Fat12::open(const Path &path, const char *name, kiv_os::NOpen_File flags, File &file) {
    file = File{};
    file.name = const_cast<char *>(name);

    int32_t cluster;

    DirItem dirItem = get_cluster(ROOT_DIR_SECTOR_START, path);
}

DirItem Fat12::get_cluster(const int startSector, const Path &path) {

    if (path.path.empty()) {
        DirItem dirItem = {"/", "", 0, ROOT_DIR_SECTOR_START};
        return dirItem;
    }

    for (int i = 0; i < path.path.size(); ++i) {


    }

    return DirItem();
}

kiv_os::NOS_Error Fat12::mkDir(Path &path, uint16_t attributes) {
    std::string folderName = path.path.back(); //posledni polozka je jmeno

    path.path.pop_back(); //cesta, tzn. bez posledni polozky

    int startSector;

    std::vector<int> sectorsIndexes;

    if (path.path.empty()) { //root
        startSector = ROOT_DIR_SECTOR_START;
        for (int i = ROOT_DIR_SECTOR_START; i < USER_DATA_START; ++i) {
            sectorsIndexes.push_back(i);
        }
    } else {
        DirItem targetFolder = getDirItemCluster(ROOT_DIR_SECTOR_START, path, fat);
        sectorsIndexes = getSectorsIndexes(fat, targetFolder.firstCluster);
        startSector = sectorsIndexes.at(0);

    }

    std::vector<DirItem> directoryItem = getItemsFromDir(fat, startSector);

    // nalezt 1 volny cluster

    int freeIndex = getFreeIndex(fat);

    if (freeIndex == -1) {
        return kiv_os::NOS_Error::Not_Enough_Disk_Space;
    } else {
       //TODO zapsat do tabulky
    }

    //TODO zapsat data

}


