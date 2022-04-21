#include "defaulthtml.h"

Helper::Helper() {
    srcDir_ = getcwd(nullptr, 256);
    FindFiles_(std::string(srcDir_), files_);
    for (std::string f : files_) {
        // cout << f.substr(0, f.size()-10) << endl;
        DEFAULT_HTML.insert(f.substr(0, f.size()-10));
    }
}

Helper* Helper::Instance() {
    static Helper inst;
    return &inst;
}

void Helper::FindFiles_(std::string root, std::vector<std::string> &files) {
    DIR* dir;
    struct dirent* ptr;
    if (!(dir = opendir(root.c_str()))) return;
    while ((ptr = readdir(dir)) != nullptr) {
        std::string sub_file = root + "/" + ptr->d_name;
        if (ptr->d_type != 4 && ptr->d_type != 8) return;
        if (ptr->d_type == 4) {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
                FindFiles_(sub_file, files);
            }
        }
        else if (ptr->d_type == 8) {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
                if (strstr(ptr->d_name, "index.html"))
                    files.push_back(sub_file);
            }
        }
    }
}