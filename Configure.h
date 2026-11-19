#pragma once

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
public:
    bool readJSON(const string& path);
    json schema;
    void createDirectories(const string& path);
};

class ReadBD : public ConfigureBD {
private:
    void PrintTable(string str);
public:
    void ReadTable(const string& tableName);
};
