//
// Created by Kuba on 20.10.2021.
//

#include <algorithm>
#include <utility>
#include "path.h"


Path::Path(std::string file_path) {
    Create_Path(file_path);
    Create_Name();
}

/**
 * Vytvori vector stringu jednotlivych slozek cesty, upravi podle "." a ".." (vynecha, resp. odstrani posledni)
 * @param file_path cesta zadana jako vektor znaku
 */
void Path::Create_Path(const std::string& file_path) {
	const std::string cur_dir = "."; // soucasny adresat
	const std::string parent_dir = ".."; // nadrazeny adresar
	const char separator = '\\'; // separator
    std::vector<char> item;
    int pos = 0;
	while (true) {
        char c = file_path[pos];
        if (c == separator) {
            std::string item_string(item.begin(), item.end());
            if (item_string == parent_dir) {
                path_vector.pop_back();
            } else if (item_string != cur_dir && !item_string.empty()) {
                std::transform(item_string.begin(), item_string.end(), item_string.begin(), ::toupper); // vsechno ve FAT12 je ukladano velkymi pismeny
                path_vector.push_back(item_string);
            }
            item.clear();
        } else if (c == '\0') {
            std::string item_string(item.begin(), item.end());
            if (item_string == parent_dir) {
                path_vector.pop_back();
            } else if (item_string != cur_dir){
                std::transform(item_string.begin(), item_string.end(), item_string.begin(), ::toupper); // vsechno ve FAT12 je ukladano velkymi pismeny
                path_vector.push_back(item_string);
            }
            break;
        } else {
            item.push_back(c);
        }
        pos++;
    }
    if (path_vector.front().find(':') != std::string::npos) { // je absolutni
        disk_letter = path_vector.front().at(0);
        path_vector.erase(path_vector.begin());
        is_relative = false;
    } else {
        is_relative = true;
    }
    //TODO osetrit mozna prazdnou cestu nebo neco takovyho
}

/**
 * Vytvori jmeno slozky/souboru a priponu
 */
void Path::Create_Name() {
	constexpr char dot = '.'; // tecka
    extension.clear();
    name.clear();
    full_name = path_vector.back();
    bool is_extension = false;
    for (const char c: full_name) {
        if (c == dot) {
            is_extension = true;
            continue;
        }
        if (is_extension) {
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
 * Prida cestu k soucasne ceste
 * @param path cesta, ktera bude pridana
 */
void Path::Append_Path(const Path &path) {
    for (const auto & i : path.path_vector) {
        path_vector.push_back(i);
    }
    Create_Name();
}

/**
 * Vrati nazev souboru/slozky do cesty
 */
void Path::Return_Name_to_Path() {
    path_vector.push_back(full_name);
}

/**
 * Prevede cestu na string
 * @return string ziskany z vektoru stringu cesty
 */
std::string Path::To_String() const {
    std::string res;
    constexpr char separator = '\\';
    if (!is_relative) {
        res += disk_letter;
        res += ':';
        res += separator;
    }
    for (const auto & i : path_vector) {
        res += i;
        res += separator;
    }
    if (res.back() == separator) {
        res.pop_back();
    }
    return res;
}
