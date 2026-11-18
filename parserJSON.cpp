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

    schema = json::parse(file);
    file.close();
}

void ConfigureBD::printJSON(){
    cout << schema.dump() << endl;
}

void ConfigureBD::createDirectories(string path){
    
    if(!readJSON(path)) return;

    string name = schema["name"].get<string>();
    filesystem::create_directory(name);

    for (auto [table_name, columns] : schema["structure"].items()){
        string path = name + '/' + table_name;
        filesystem::create_directory(path);
        path += "/1.csv";
        if(!filesystem::exists(path)){
            ofstream file(path);
            if (!file.is_open()){
                cerr << "ERROR: unable create file\n";
                return;
            }
            file << name << "_pk,";
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