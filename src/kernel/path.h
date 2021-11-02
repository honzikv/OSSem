//
// Created by Kuba on 10.10.2021.
//

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
    Path(const char *file_path);
    void Delete_Name_From_Path();
};