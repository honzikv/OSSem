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

kiv_os::NOS_Error Fat12::open(Path &path, kiv_os::NOpen_File flags, File &file, uint8_t attributes) {
    file = File{};
    std::string fileName = path.path.back();
    file.name = &fileName.at(0);

    std::vector<std::string> pathCopy(path.path);

    //TODO jenom na konci - to by mela byt chyba '.'
    DirItem dirItem = getDirItemCluster(ROOT_DIR_SECTOR_START, path, fat);

    int32_t targetCluster = dirItem.firstCluster;

    kiv_os::NOS_Error res;

    if (targetCluster == -1) { // nenalezen - tzn. neexistuje
        if (flags == kiv_os::NOpen_File::fmOpen_Always) { // musi existovat, aby byl otebrem => chyba
            return kiv_os::NOS_Error::File_Not_Found;
        } else { // vytvoreni souboru
            dirItem.fileSize = 0;
            dirItem.attribute = attributes;

            // slozka
            if (attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
                attributes == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
                if (fileName.size() > FILE_NAME_SIZE) { // nazev max 8 znaku
                    return kiv_os::NOS_Error::Invalid_Argument;
                }
                res = mkDir(path, attributes);
            } else { // soubor
                if (!validateFileName(fileName)) {
                    return kiv_os::NOS_Error::Invalid_Argument;
                }

                file.size = 0;
                res = createFile(path, attributes, fat);
            }

            if (res == kiv_os::NOS_Error::Not_Enough_Disk_Space) {
                return res;
            } else {
                targetCluster = getDirItemCluster(ROOT_DIR_SECTOR_START, path, fat).firstCluster;
            }
        }
    } else if (((attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory)) &&
                (dirItem.attribute & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory))) == 0) {
        // neni atribut slozka - chybi nastaveny bit 0x10
        return kiv_os::NOS_Error::Permission_Denied;
    }
    // soubor/slozka existuje (pripadne vytvoren(a))

    file.handle = targetCluster;
    file.attributes = dirItem.attribute;

    // zjistenu, jestli je to slozka
    if (dirItem.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ||
        dirItem.attribute == static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID)) {
        std::vector<kiv_os::TDir_Entry> directoryEntries = readDirectory(path, fat);
        dirItem.fileSize = directoryEntries.size() * sizeof(kiv_os::TDir_Entry);
    }
    file.size = dirItem.fileSize;

    return kiv_os::NOS_Error::Success;
}

/**
 * Vytvori novou slozku
 * @param path cesta k slozce
 * @param attributes atributy slozky
 * @return zpravu o uspechu / neuspechu
 */
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

    std::vector<DirItem> directoryItems = getFoldersFromDir(fat, startSector);

    // nalezt 1 volny cluster
    int freeIndex = getFreeIndex(fat);

    if (freeIndex == -1) {
        return kiv_os::NOS_Error::Not_Enough_Disk_Space;
    } else {
        writeValueToFat(fat, freeIndex, END_CLUSTER_INT); // zapise do FAT konec clusteru
        saveFat(fat);
    }

    std::vector<unsigned char> bufferToWrite;
    std::vector<char> bufferToSave;

    // jmeno souboru
    int i = 0;

    while (i < folderName.size()) {
        bufferToWrite.push_back(folderName.at(i));
        i++;
    }

    while (i < FILE_NAME_SIZE) { //doplnit na 8 bytu
        bufferToWrite.push_back(' '); //TODO constanta
        i++;
    }

    // pripona - slozka nema zadnou
    for (int j = 0; j < FILE_EXTENSION_SIZE; ++j) {
        bufferToWrite.push_back(' '); //TODO const
    }

    bufferToWrite.push_back(attributes);

    // nedulezite (cas vytvoreni atd.)
    for (int j = 0; j < DIR_ITEM_ATTR_TO_CLUSTER; ++j) {
        bufferToWrite.push_back(' '); //TODO const
    }

    // cislo clusteru
    std::vector<unsigned char> bytesFromClusterNum = getBytesFromInt(freeIndex);

    for (unsigned char &j: bytesFromClusterNum) {
        bufferToWrite.push_back(j);
    }

    // velikost souboru - pro slozku 0
    for (int j = 0; j < DIR_ITEM_FILE_SIZE_BYTES; ++j) {
        bufferToWrite.push_back(0);
    }
    //TODO check na .. v rootu a zapsat to
    if (path.path.empty()) { // slozka v rootu
        if ((directoryItems.size() + 2) <= (sectorsIndexes.size() * MAX_ITEMS_CLUSTER)) { // + 2 je . a nova
            size_t clusterPos =
                    (directoryItems.size() + 1) / MAX_ITEMS_CLUSTER; // poradi clusteru - jeden pojme 16 polozek
            size_t itemPos = (directoryItems.size() + 1) % MAX_ITEMS_CLUSTER; // poradi v ramci clusteru

            std::vector<unsigned char> clusterData = readFromRegisters(1, SECTOR_SIZE, sectorsIndexes.at(clusterPos) -
                                                                                       DATA_SECTOR_CONVERSION);

            for (int j = 0; j < bufferToWrite.size(); ++j) {
                clusterData.at(itemPos * 32 + j) = bufferToWrite.at(j); //TODO 32 const
            }

            for (unsigned char &j: clusterData) {
                bufferToSave.push_back(j);
            }

            writeToRegisters(bufferToSave, sectorsIndexes.at(clusterPos) - DATA_SECTOR_CONVERSION);
        } else { // nevejde se do root - uvolnit a konec
            writeValueToFat(fat, freeIndex, 0); // uvolni misto
            saveFat(fat);
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        }
    } else {
        bool canAdd = true;

        if ((directoryItems.size() + 3) > (sectorsIndexes.size() * MAX_ITEMS_CLUSTER)) { // + 2 je ., .. a nova
            int newClusterPos = allocateNewCluster(startSector, fat);
            if (newClusterPos == -1) {
                canAdd = false;
            } else {
                sectorsIndexes.push_back(newClusterPos);
            }
        }

        if (canAdd) { //TODO 2 const prozkoumat
            size_t clusterPos =
                    (directoryItems.size() + 2) / MAX_ITEMS_CLUSTER; // poradi clusteru - jeden pojme 16 polozek
            size_t itemPos = (directoryItems.size() + 2) % MAX_ITEMS_CLUSTER; // poradi v ramci clusteru

            std::vector<unsigned char> clusterData = readFromRegisters(1, SECTOR_SIZE, sectorsIndexes.at(clusterPos) -
                                                                                       DATA_SECTOR_CONVERSION);

            for (int j = 0; j < bufferToWrite.size(); ++j) {
                clusterData.at(itemPos * 32 + j) = bufferToWrite.at(j);
            }

            for (unsigned char &j: clusterData) {
                bufferToSave.push_back(j);
            }

            writeToRegisters(bufferToSave, sectorsIndexes.at(clusterPos) - DATA_SECTOR_CONVERSION);
        } else {
            writeValueToFat(fat, freeIndex, 0); // uvolni misto
            saveFat(fat);
            return kiv_os::NOS_Error::Not_Enough_Disk_Space;
        }
    }
    return kiv_os::NOS_Error::Success;

}


