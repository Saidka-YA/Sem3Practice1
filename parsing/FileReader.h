#pragma once
#include <filesystem>
#include <fstream>
#include "../struct/Vector.hpp"
namespace fs = filesystem;
using namespace std;

class FileReader {
private:
    fs::path path;
    ifstream file;
    int currentFile;
    bool finished = false;
    bool readError = false;
    bool openCsv(const fs::path& csvPath);
public:
    FileReader(fs::path path) : path(path), currentFile(1) {
        openCsv(this->path / "1.csv");
    }

    bool nextRow(string& line);
    void readLine(string& line, Vector<string>& fields);
    bool hasReadError() const {
        return readError;
    }
};
