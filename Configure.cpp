#include "Configure.h"
#include <filesystem>
#include <fstream>

bool ConfigureBD::readJSON(const string& path){
    ifstream file(path);

    if (!file.is_open()){
        cerr << "ERROR: unable read fil\n";
        return false;
    }

    try {
        schema = json::parse(file);
    } catch (json::parse_error) {
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

void ConfigureBD::createDirectories(const string& path){
    
    if(!readJSON(path)) return;

    string name = schema["name"].get<string>();
    fs::create_directory(name);

    for (auto [tableName, columns] : schema["structure"].items()){
        fs::create_directory(fs::path(name) / tableName);
        if (!fs::exists( name + "/" + tableName + "/" + tableName + "_pk_sequence")) {
            ofstream file (name + "/" + tableName + "/" + tableName + "_pk_sequence");
            if (!file.is_open()) {
                cerr << "ERROR: unable create file\n";
                return;
            }
            file << "0";
            file.close();
        }
        if(!fs::exists(fs::path(name) / tableName / "1.csv")){
            ofstream file(fs::path(name) / tableName / "1.csv");
            if (!file.is_open()){
                cerr << "ERROR: unable create file\n";
                return;
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
}

void ReadBD::ReadTable(const string& tableName) {
    string path = schema["name"].get<string>() + "/" + tableName;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
           path = entry.path();
           ifstream file(path);
           if (!file.is_open()) {
                cerr << "ERROR: unable read file\n";
                return;
            }
            string line;
            while (getline(file, line)) {
                PrintTable(line);
            }
        }
    }
}

void ReadBD::PrintTable(string str) {
    cout << str << endl;
}

int main() {
    ReadBD someBD;
    someBD.createDirectories("/home/saidka/ZALUPA/schema.json");
    someBD.ReadTable("Tablica1");
    return 0;
}