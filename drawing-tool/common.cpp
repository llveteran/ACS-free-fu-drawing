#include "common.hpp"

using namespace std;

void sleep_ms(const int ms) {
    this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void listdir(const string path, vector<string> &files) {
    files.clear();

    HANDLE hFind;
    WIN32_FIND_DATA findData;
    LARGE_INTEGER size;
    hFind = FindFirstFile(path.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        // 忽略"."和".."两个结果 
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    // 是否是目录 
        {
            // pass
        } else {
            size.LowPart = findData.nFileSizeLow;
            size.HighPart = findData.nFileSizeHigh;
            files.push_back(string(findData.cFileName));
        }
    } while (FindNextFile(hFind, &findData));
}