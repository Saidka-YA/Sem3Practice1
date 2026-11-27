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

    struct FieldIndex {
        size_t table;
        size_t field;
    };

    struct IndexedCondition {
        FieldIndex index;
        FieldIndex rightIndex;
        string expectedValue;
        bool expectedIsColumn;
    };

    bool findField(const Vector<string>& tableNames, const string& name, FieldIndex& index);
    const string& fieldValue(const Vector<Vector<string>>& currentRows, const FieldIndex& index);
    bool matchesWhere(const Vector<Vector<IndexedCondition>>& groups,
                      const Vector<Vector<string>>& currentRows);
    bool scan(size_t level, const Vector<string>& tableNames, const Vector<fs::path>& paths,
              const Vector<FieldIndex>& indexes, const Vector<Vector<IndexedCondition>>& groups,
              Vector<Vector<string>>& currentRows);
public:
    bool insert(const string& tableName, const Vector<string>& values);
    SQLcommands(ConfigureBD& config) : config(config) {}
    bool matches(const string& tableName, const Vector<string>& fields, const Condition& condition);
    bool deleteRows(const string& tableName, const WhereClause& where);
    bool matches(const string& tableName, const Vector<string>& fields, const WhereClause& where);
    bool select(const Vector<string>& tableNames, const WhereClause* where, const Vector<string>& columns);
};
