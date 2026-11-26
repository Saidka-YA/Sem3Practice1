#pragma once
#include "../Configure/Configure.h"
#include "../struct/Vector.hpp"
#include "../parsing/FileReader.h"
#include <string>
#include <fstream>

struct Condition {
    string columnName;
    string expectedValue;
    bool expectedIsColumn = false;
};

struct WhereClause {
    Vector<Vector<Condition>> groups;
};

class SQLcommands {
private:
    ConfigureBD& config;
public:
    bool insert(const string& tableName, const Vector<string>& values);
    SQLcommands(ConfigureBD& config) : config(config) {}
    bool matches(const string& tableName, const Vector<string>& fields, const Condition& condition);
    bool select(const string& tableName, const WhereClause* where, const Vector<string>& columns);
    bool deleteRows(const string& tableName, const WhereClause& where);
    bool matches(const string& tableName, const Vector<string>& fields, const WhereClause& where);
    bool select(const Vector<string>& tableNames, const WhereClause* where, const Vector<string>& columns);
};
