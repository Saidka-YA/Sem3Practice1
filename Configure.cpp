#include "Configure.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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

void ConfigureBD::ReadTable(const string& tableName) {
    if (!schema.is_object()) return;
    if (schema["structure"].count(tableName) <= 0) {
        cerr << "ERROR: Unable read table\n";
        return;
    }
    string path = schema["name"].get<string>() + "/" + tableName;
    if (!fs::exists(path + "/" + "1.csv")) {
        cerr << "ERROR: unble read file\n";
        return;
    }
    for (int i = 1; i < INT_MAX; i++) {
        if (!fs::exists(path + "/" + to_string(i) + ".csv")) {
            return;
        }
        ifstream file (path + "/" + to_string(i) + ".csv");
        if (!file.is_open()) {
            cerr << "ERROR: unable read file\n";
            return;
        }
        string zagolovok;
        getline(file, zagolovok);
        // Допилить логику
        string line;
        while (getline(file, line)) {
            // Заглушка
        }
    }
    
}

void ConfigureBD::PrintTable(string str) { // Временная функция для тестирования
    cout << str << endl;
}

bool Reader::nextRow(string& line) {
    while (true) {
        if (getline(file, line)) {
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

void Reader::readLine(string& line, const char& separator){
    stringstream ss(line);
    string token;
    while (getline(ss, token, separator)) {
        cout << token << endl;
    }
    
}

int main() {
    ConfigureBD someBD;
    someBD.createDirectories("/home/saidka/ZALUPA/schema.json");
    someBD.ReadTable("Tablica1");

    Reader read1("/home/saidka/ZALUPA/Schema 1/Tablica1");
    string str;
    while (read1.nextRow(str)) {
        read1.readLine(str, ',');
    }
    return 0;
}