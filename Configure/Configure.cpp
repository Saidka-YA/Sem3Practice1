#include "./Configure.h"


bool ConfigureBD::readJSON(const string& path){
    ifstream file(path);

    if (!file.is_open()){
        cerr << "ERROR: unable read fil\n";
        return false;
    }

    try {
        schema = json::parse(file);
    } catch (const json::parse_error&) {
        cerr << "ERROR: unable parse json\n";
        return false;
    }
    file.close();

    if (!schema.is_object()) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }
    if (!schema.contains("name") || !schema["name"].is_string()) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }
    if (!schema.contains("tuples_limit") || !schema["tuples_limit"].is_number_integer() || schema["tuples_limit"] <= 0) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }
    if (!schema.contains("structure") || !schema["structure"].is_object()) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }

    return true;
}

bool ConfigureBD::createDirectories(const string& path){
    
    if(!readJSON(path)) return false;

    string name = schema["name"].get<string>();
    fs::create_directory(name);

    for (auto [tableName, columns] : schema["structure"].items()){
        fs::create_directory(fs::path(name) / tableName);
        if (!fs::exists( name + "/" + tableName + "/" + tableName + "_pk_sequence")) {
            ofstream file (name + "/" + tableName + "/" + tableName + "_pk_sequence");
            if (!file.is_open()) {
                cerr << "ERROR: unable create file\n";
                return false;
            }
            file << 0;
            file.close();
        }
        if (!fs::exists( name + "/" + tableName + "/" + tableName + "_lock")) {
            ofstream file (name + "/" + tableName + "/" + tableName + "_lock");
            if (!file.is_open()) {
                cerr << "ERROR\n";
                return false;
            }
            file.close();
        }
        if(!fs::exists(fs::path(name) / tableName / "1.csv")){
            ofstream file(fs::path(name) / tableName / "1.csv");
            if (!file.is_open()){
                cerr << "ERROR: unable create file\n";
                return false;
            }
            file << tableName << "_pk,";
            bool first = true;
            for (string name : columns) {
                if (!first) {
                    file << ",";
                }
                file << name;
                first = false;
            }
            file << '\n';
            file.close();
        }
    }
    return true;
}

string ConfigureBD::databaseName() {
    return schema["name"].get<string>();
}

int ConfigureBD::tuplesLimit() {
    return schema["tuples_limit"].get<int>();
}

size_t ConfigureBD::countColumns(const string& tableName) {
    return schema["structure"][tableName].size();
}

string ConfigureBD::columnName(const string& tableName, size_t index) const {
    return schema["structure"][tableName][index].get<string>();
}

TableLock::TableLock(const fs::path& lockPath) {
    fd = open(lockPath.c_str(), O_RDWR | O_CLOEXEC);
    if (fd == -1) {
        return;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        close(fd);
        fd = -1;
    }
}

TableLock::~TableLock() {
    if (fd != -1) {
        close(fd);
    }
}

bool TableLock::isLocked() const {
    return fd != -1;
}
