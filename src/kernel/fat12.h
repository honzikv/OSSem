//
// Created by Kuba on 09.10.2021.
//

#ifndef OS_FAT12_H
#define OS_FAT12_H


#include <string>
#include "vfs.h"
#include "../api/hal.h"

const int FAT_TABLE_SECTOR_COUNT = 9;
const int SECTOR_SIZE = 512;
const int DISK_NUM = 129;
const int ROOT_DIR_SECTOR = 19;

struct DirItem {
    std::string fileName;
    std::string fileExtension;
    size_t fileSize;
    int firstCluster;
};

class Fat12 : public VFS {

    Fat12();

    //TODO doimplementovat
    virtual kiv_os::NOS_Error open(const Path &path, const char *name, kiv_os::NOpen_File flags, File &file) override;

    virtual kiv_os::NOS_Error close(File file) override;

    virtual kiv_os::NOS_Error readDir(const Path &path, std::vector<kiv_os::TDir_Entry> &entries) override;

    virtual kiv_os::NOS_Error mkDir(const Path &path, uint16_t attributes) override;

    virtual kiv_os::NOS_Error rmDir(const Path &path) override;

    virtual kiv_os::NOS_Error read(File file, size_t size, size_t offset, std::vector<char> &out) override;

    virtual kiv_os::NOS_Error
    write(File file, size_t size, size_t offset, std::vector<char> buffer, size_t &written) override;

    static DirItem get_cluster(const int startSector, const Path &path);
};


#endif //OS_FAT12_H
