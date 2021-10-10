//
// Created by Kuba on 09.10.2021.
//

#include "fat12.h"
#include "fat_helper.h"

std::vector<unsigned char> fatTable;
std::vector<unsigned char> secondFatTable;


Fat12::Fat12() {
    fatTable = loadFatTable(1);
    secondFatTable = loadFatTable(1 + FAT_TABLE_SECTOR_COUNT);
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


