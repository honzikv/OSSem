//
// Created by Kuba on 10.10.2021.
//

#pragma once

#include <vector>
#include <string>

class Path {
private:
    void Create_Path(const std::string& file_path);

    void Create_Name();

public:
    std::vector<std::string> path_vector;
    std::string full_name;
    std::string name;
    std::string extension;
    bool is_relative = false;
    std::string disk_letter;

    explicit Path(std::string file_path);

    void Delete_Name_From_Path();

    void Return_Name_to_Path();

    void Append_Path(const Path &path);

    std::string To_String();


};