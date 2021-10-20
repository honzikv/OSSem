//
// Created by Kuba on 20.10.2021.
//

#include "path.h"

Path::Path(const char *file_path) {
    CreatePath(file_path);
    CreateName();
}

void Path::CreatePath(const char *file_path) {
    std::vector<char> item;
    int pos = 0;
    char c;
    while (true) {
        c = file_path[pos];
        //TODO co treba '\'
        if (c == '/') {
            std::string item_string(item.begin(), item.end());
            if (!item_string.empty()) {
                path_vector.push_back(item_string);
            }
            item.clear();
        } else if (c == '\0') {
            std::string item_string(item.begin(), item.end());
            path_vector.push_back(item_string);
            break;
        } else {
            item.push_back(c);
        }
        pos++;
    }
    //TODO osetrit mozna prazdnou cestu nebo neco takovyho
}

void Path::CreateName() {
    full_name = path_vector.back();
    bool isExtension = false;
    for (char c : full_name) {
        if (c == '.') { //TODO const
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

