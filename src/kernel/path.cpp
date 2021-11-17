//
// Created by Kuba on 20.10.2021.
//

#include "path.h"

const char kSeparator = '\\';
const char kDot = '.';

Path::Path(const char *file_path) {
    Create_Path(file_path);
    Create_Name();
}

void Path::Create_Path(const char *file_path) {
    std::vector<char> item;
    int pos = 0;
    char c;
    while (true) {
        c = file_path[pos];
        if (c == kSeparator) {
            std::string item_string(item.begin(), item.end());
            if (!item_string.empty()) {
                for (char &ch: item_string) { // vsechno ve FAT12 je ukladano velkymi pismeny
                    ch = ::toupper(ch);
                }
                path_vector.push_back(item_string);
            }
            item.clear();
        } else if (c == '\0') {
            std::string item_string(item.begin(), item.end());
            for (char &ch: item_string) { // vsechno ve FAT12 je ukladano velkymi pismeny
                ch = ::toupper(ch);
            }
            path_vector.push_back(item_string);
            break;
        } else {
            item.push_back(c);
        }
        pos++;
    }
    //TODO osetrit mozna prazdnou cestu nebo neco takovyho
}

/**
 * Vytvori jmeno slozky/souboru a priponu
 */
void Path::Create_Name() {
    full_name = path_vector.back();
    bool isExtension = false;
    for (const char c: full_name) {
        if (c == kDot) {
            isExtension = true;
            continue;
        }
        if (isExtension) {
            extension.push_back(c);
        } else {
            name.push_back(c);
        }
    }
}

/**
 * Odstrani nazev souboru/slozky z cesty
 */
void Path::Delete_Name_From_Path() {
    path_vector.pop_back();
}

/**
 * Vrati nazev zpet do cesty
 */
void Path::Add_Name_To_Path() {
    path_vector.push_back(full_name);
}

