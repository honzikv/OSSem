//
// Created by Kuba on 10.10.2021.
//

#pragma once

#include <vector>
#include <string>

class Path {
private:
    void Create_Path(const char *file_path);

    void Create_Name();

    const std::string kCurDir = "."; // soucasny adresat
    const std::string kParentDir = ".."; // nadrazeny adresar
    const char kSeparator = '\\'; // separator
    const char kDot = '.'; // tecka

public:
    std::vector<std::string> path_vector;
    std::string full_name;
    std::string name;
    std::string extension;

    explicit Path(const char *file_path);

    void Delete_Name_From_Path();

};