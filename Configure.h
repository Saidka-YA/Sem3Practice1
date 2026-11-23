#pragma once

#include <cinttypes>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <limits.h>
#include <sstream>
using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;

class ConfigureBD {    
private:
    void PrintTable(string str);
   json schema;
public:
    bool readJSON(const string& path);
    void createDirectories(const string& path);
    void ReadTable(const string& tableName);
};

class Reader {
private:
    fs::path path;
    ifstream file;
    int currentFile;
    bool finished = false;
public:
    Reader(fs::path path) : path(path) {
        currentFile = 1;
        file.open(path / "1.csv" );
        if (!file.is_open()) {return;};
        string zagolovok;
        getline(file, zagolovok);        
    }

    bool nextRow(string& line);
    void readLine(string& line, const char& separator);
};