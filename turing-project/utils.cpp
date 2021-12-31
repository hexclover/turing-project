#include "utils.h"
#include <errno.h>
#include <fstream>
#include <utility>
#define BUFSIZE 65536

string replicate(string s, size_t n) {
    string res;
    res.reserve(s.length() * n);
    for (size_t _ = 0; _ < n; ++_)
        res.append(s);
    return res;
}

vector<string> split(string s, char delim) {
    vector<string> res = {""};
    for (auto c : s) {
        if (c == delim) {
            res.emplace_back();
        } else {
            res.back().push_back(c);
        }
    }
    return res;
}

Either<FileError, string> readFile(string path) {
    std::ifstream file(path, std::ifstream::in);
    if (!file) {
        switch (errno) {
        case EPERM:
        case EACCES:
            return Either<FileError, string>::inl(RF_PERM);
            break;
        case ENOENT:
            return Either<FileError, string>::inl(RF_NOTFOUND);
            break;
        default:
            return Either<FileError, string>::inl(RF_OTHER);
            break;
        }
    }
    string res;
    char buf[BUFSIZE+1];
    do {
        file.read(buf, BUFSIZE);
        buf[file.gcount()] = '\0';
        res.append(buf);
    } while (file);
    return Either<FileError, string>::inr(std::move(res));
}

Either<FileError, string> saveToFile(string contents, string path) {
    throw "Not implemented";
}
