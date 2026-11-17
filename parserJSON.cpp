#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
using namespace std;
using json = nlohmann::json;


class ConfigureBD{
private:
    json schema;
    void readJSON(string path);
public:
    void createDirectories(string path);
    void printJSON();
};

void ConfigureBD::readJSON(string path){
    ifstream file(path);

    if (!file.is_open()){
        cerr << "ERROR: unable read fil\n";
        return;
    }

    schema = json::parse(file);
    file.close();
}

void ConfigureBD::printJSON(){
    cout << schema.dump() << endl;
}

void ConfigureBD::createDirectories(string path){
    readJSON(path);
    
    string name = schema["name"].get<string>();
    filesystem::create_directory(name);

    for (auto [table_name, colimns] : schema["structure"].items()){
        string path = name + '/' + table_name;
        filesystem::create_directory(path);
        path += "/1.csv";
        ofstream file(path);
        if (!file.is_open()){
            cerr << "ERROR: unable create file\n";
            return;
        }
        file.close();
    }
}

int main(){
    ConfigureBD config;
    config.createDirectories("/home/saidka/ZALUPA/schema.json");
    return 0;
}