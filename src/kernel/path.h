//
// Created by Kuba on 10.10.2021.
//

#include <vector>
#include <string>

class Path {
private:
    void CreatePath(const char *file_path);
    void CreateName();
public:
    std::vector<std::string> path_vector;
    std::string full_name;
    std::string name;
    std::string extension;
    Path(const char *file_path);
};