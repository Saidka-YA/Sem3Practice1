#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
using namespace std;
using json = nlohmann::json;


class ConfigureBD{
private:
    json schema;
    bool readJSON(string path);
public:
    void createDirectories(string path);
    void printJSON();
};

bool ConfigureBD::readJSON(string path){
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
    if (!schema["name"].is_string()) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }
    if (!schema["tuples_limit"].is_number_integer()) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }
    if (!schema["structure"].is_array()) {
        cerr << "ERROR: unvalid configuration\n";
        return false;
    }

    return true;
}

void ConfigureBD::printJSON(){
    cout << schema.dump() << endl;
}

void ConfigureBD::createDirectories(string path){
    
    if(!readJSON(path)) return;

    string name = schema["name"].get<string>();
    filesystem::create_directory(name);

    for (auto [tableName, columns] : schema["structure"].items()){
        string path = name + '/' + tableName;
        filesystem::create_directory(path);
        path += "/1.csv";
        if(!filesystem::exists(path)){
            ofstream file(path);
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

int main(){
    ConfigureBD config;
    config.createDirectories("/home/saidka/ZALUPA/schema.json");
    return 0;
}