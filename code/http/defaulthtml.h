#ifndef DEFAULT_HTML_H
#define DEFAULT_HTML_H

#include <unistd.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <cstring>

class Helper {
public:
    std::unordered_set<std::string> DEFAULT_HTML;
    static Helper* Instance();

private:
    Helper();
    ~Helper() = default;

    void FindFiles_(std::string root, std::vector<std::string> &files);

    std::string srcDir_;
    std::vector<std::string> files_;
    // std::unordered_set<std::string> DEFAULT_HTML;
};

#endif