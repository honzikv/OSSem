//
// Created by Kuba on 20.10.2021.
//

#include "path.h"


Path::Path(const char *file_path) {
    Create_Path(file_path);
    Create_Name();
}

/**
 * Vytvori vector stringu jednotlivych slozek cesty, upravi podle "." a ".." (vynecha, resp. odstrani posledni)
 * @param file_path cesta zadana jako vektor znaku
 */
void Path::Create_Path(const char *file_path) {
    std::vector<char> item;
    int pos = 0;
    char c;
    while (true) {
        c = file_path[pos];
        if (c == kSeparator) {
            std::string item_string(item.begin(), item.end());
            if (item_string == kParentDir) {
                path_vector.pop_back();
            } else if (item_string != kCurDir && !item_string.empty()) {
                for (char &ch: item_string) { // vsechno ve FAT12 je ukladano velkymi pismeny
                    ch = ::toupper(ch);
                }
                path_vector.push_back(item_string);
            }
            item.clear();
        } else if (c == '\0') {
            std::string item_string(item.begin(), item.end());
            if (item_string == kParentDir) {
                path_vector.pop_back();
            } else if (item_string != kCurDir){
                for (char &ch: item_string) { // vsechno ve FAT12 je ukladano velkymi pismeny
                    ch = ::toupper(ch);
                }
                path_vector.push_back(item_string);
            }
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
