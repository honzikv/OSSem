
#include <vector>

class Path {
public:
    std::vector<std::string> path;
    //TODO do cpp file
    Path(const char *file_path) {
        std::vector<char> item;
        int pos = 0;
        char c;
        while (true) {
            c = file_path[pos];
            //TODO co treba '\'
            if (c == '/') {
                std::string item_string(item.begin(), item.end());
                if (!item_string.empty()) {
                    path.push_back(item_string);
                }
                item.clear();
            } else if (c == '\0') {
                std::string item_string(item.begin(), item.end());
                path.push_back(item_string);
                break;
            } else {
                item.push_back(c);
            }
            pos++;
        }
        //TODO osetrit mozna prazdnou cestu nebo neco takovyho
    }
};