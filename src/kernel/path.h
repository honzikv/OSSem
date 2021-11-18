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

public:
    std::vector<std::string> path_vector;
    std::string full_name;
    std::string name;
    std::string extension;

    explicit Path(const char *file_path);

    void Delete_Name_From_Path();

    void Add_Name_To_Path();
};