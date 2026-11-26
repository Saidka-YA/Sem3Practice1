#include "../SQL/SQL.h"
#include "../Configure/Configure.h"
#include "../parsing/FileReader.h"
#include "../parsing/SQLparser.h"

#pragma once

class ConsoleApp {
public:
    int run(const string& pathToShema) {
        ConfigureBD config;
        try {
            if (!config.createDirectories(pathToShema)) {
                cerr << "Не удалось подготовить БД: " << pathToShema << '\n';
                return 1;
            }
        } catch (const fs::filesystem_error& error) {
            cerr << "Ошибка файловой системы: " << error.what() << '\n';
            return 1;
        } catch (const json::exception& error) {
            cerr << "Ошибка в схеме БД: " << error.what() << '\n';
            return 1;
        }

        SQLcommands sql(config);
        SQLParser parser(sql);

        string input;
        while (getline(cin, input)) {
            if (input == "exit") {
                break;
            }

            if (input.empty() || input.find_first_not_of(" \t\r\n") == string::npos){
                continue;
            }

            if (!parser.execute(input)) {
                cerr << "ERROR: failed query: " << input << '\n';
            }
        }

        return 0;
    }
};
