//
// Created by Kuba on 09.10.2021.
//

#include "fat12.h"
std::vector<unsigned char> fatTable;
std::vector<unsigned char> secondFatTable;

void loadFatTable(std::vector<unsigned char> specificFatTable, int startIndex) {
    kiv_hal::TRegisters registers;
    kiv_hal::TDisk_Address_Packet addressPacket;

    addressPacket.count = FAT_TABLE_SECTOR_COUNT;

    int sectorSize = SECTOR_SIZE * FAT_TABLE_SECTOR_COUNT;

    auto sector = new std::string[sectorSize];
    addressPacket.sectors = (void *) sector;
    addressPacket.lba_index = 1;

    registers.rdx.l = DISK_NUM;
    registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
    registers.rdi.r = reinterpret_cast<decltype(registers.rdi.r)>(&addressPacket);

    kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, registers); //TODO error

    char *buffer = reinterpret_cast<char *>(addressPacket.sectors);

    for (int i = 0; i < sectorSize; i++) {
        specificFatTable.push_back(buffer[i]);
    }
}

Fat12::Fat12() {
    loadFatTable(fatTable, 1);
    loadFatTable(secondFatTable, 1 + FAT_TABLE_SECTOR_COUNT);
}

kiv_os::NOS_Error Fat12::open(const Path &path, const char *name, kiv_os::NOpen_File flags, File &file) {
    file = File{};
    file.name = const_cast<char *>(name);

    int32_t cluster;

    DirItem dirItem = get_cluster(ROOT_DIR_SECTOR, path);
}

DirItem Fat12::get_cluster(const int startSector, const Path &path) {

    if (path.path.empty()) {
        DirItem dirItem = {"/", "", 0, ROOT_DIR_SECTOR};
        return dirItem;
    }

    for (int i = 0; i < path.path.size(); ++i) {


    }

    return DirItem();
}


