//
// Created by Kuba on 10.10.2021.
//

#pragma once

#include <vector>
#include <string>

class Path {
private:
    void Create_Path(std::string file_path);

    void Create_Name();

public:
    std::vector<std::string> path_vector;
    std::string full_name;
    std::string name;
    std::string extension;
    bool is_relative = false;
    char disk_letter = '\0';

    explicit Path(std::string file_path);

    void Delete_Name_From_Path();

    void Append_Path(const Path &path);

    std::string To_String();


};