#pragma once

#include <cinttypes>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <limits.h>
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
        path /= "1.csv";
        file.open(path);
        if (!file.is_open()) {return;};
        string zagolovok;
        getline(file, zagolovok);        
    }
    bool nextRow(string& row) {
        while (true) {
            if (getline(file, row)) {
                return true;
            }

            file.close();
            ++currentFile;

            fs::path nextPath = path / (to_string(currentFile) + ".csv");
            if (!fs::exists(nextPath)) {
                finished = true;
                return false;
            }

            file.open(nextPath);
            if(!file.is_open()) {
                finished = true;
                return false;
            }

            string header;
            if (!getline(file, header)){
                finished = true;
                return false;
            }
        }
    }
};