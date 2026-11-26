#pragma once
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <filesystem>
using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;

class ConfigureBD {    
private:
   json schema;
public:
    bool readJSON(const string& path);
    bool createDirectories(const string& path);
    string databaseName();
    int tuplesLimit();
    size_t countColumns(const string& tableName);
    string columnName(const string& tableName, size_t index) const;
};

class TableLock {
private:
    int fd = -1;

public:
    explicit TableLock(const fs::path& lockPath);
    ~TableLock();

    TableLock(const TableLock&) = delete;
    TableLock& operator=(const TableLock&) = delete;

    bool isLocked() const;
};