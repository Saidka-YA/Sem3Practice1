#include "./SQL.h"

bool SQLcommands::insert(const string& tableName, const Vector<string>& values) {
    if (values.Size() != config.countColumns(tableName)) {
        return false;
    }

    fs::path pathToTable = fs::path(config.databaseName()) / tableName;

    TableLock tableLock(pathToTable / (tableName + "_lock"));
    if (!tableLock.isLocked()) {
        cerr << "ERROR: unable to lock table: " << tableName << '\n';
        return false;
    }

    fs::path sequencePath = pathToTable / (tableName + "_pk_sequence");
    ifstream readPKSequence(sequencePath);
    if (!readPKSequence.is_open()) {
        cerr << "ERROR\n";
        return false;
    }
    int key; 
    if (!(readPKSequence >> key)) {
        cerr << "ERROR\n";
        return false;
    }
    readPKSequence.close();
    int newKey = key + 1;

    int fileNumber = 1;
    fs::path csvPath;
    while (true) {
        csvPath = pathToTable / fs::path(to_string(fileNumber) + ".csv");
        if (!fs::exists(csvPath)) {
            ofstream file(csvPath);
            if (!file.is_open()) {
                cerr << "ERROR\n";
                return false;
            }
            file << tableName << "_pk,";
            bool first = true;
            for (size_t i = 0; i < config.countColumns(tableName); i++) {
                if (!first) {
                    file << ',';
                }
                file << config.columnName(tableName, i);
                first = false;
            }
            file << '\n';
            file.close();
            break;
        }
        ifstream tableFile(csvPath);
        if (!tableFile.is_open()) {
            cerr << "ERROR\n";
            return false;
        }
        string header; getline(tableFile, header);
        int count = 0;
        string line;
        while (getline(tableFile, line)) {
            count++;
        }
        if (count < config.tuplesLimit()) {
            break;
        } 
        fileNumber++;
    }

    ofstream tableFile(csvPath, ios::app);
    if (!tableFile.is_open()) {
        cerr << "ERROR\n";
        return false;
    }
    tableFile << to_string(newKey) << ',';
    bool first = true;
    for (size_t i = 0; i < values.Size(); i++) {
        if (!first) {
            tableFile << ',';
        }
        bool quote = values[i].find_first_of(",\"") != string::npos;
        if (quote) tableFile << '"';
        for (char ch : values[i]) {
            if (ch == '"') tableFile << '"';
            tableFile << ch;
        }
        if (quote) tableFile << '"';
        first = false;
    }
    tableFile << '\n';

    tableFile.flush();
    if (!tableFile) {
        cerr << "ERROR\n";
        return false;
    }
    
    tableFile.close();

    if (tableFile.fail()) {
        cerr << "ERROR\n";
        return false;
    }
    
    ofstream writePKSequence(pathToTable / (tableName + "_pk_sequence"));
    if (!writePKSequence.is_open()) {
        cerr << "ERROR\n";
        return false;
    }

    writePKSequence << newKey;

    writePKSequence.flush();
    if (!writePKSequence) {
        cerr << "ERROR\n";
        return false;
    }

    writePKSequence.close();

    if (writePKSequence.fail()) {
        cerr << "ERROR\n";
        return false;
    }

    return true;
}

bool SQLcommands::matches(const string& tableName, const Vector<string>& fields, const Condition& condition) {
    auto fieldValue = [&](const string& columnName) -> const string* {
        if (columnName == tableName + "_pk") {
            return &fields[0];
        }

        for (size_t i = 0; i < config.countColumns(tableName); i++) {
            if (columnName == config.columnName(tableName, i)) {
                return &fields[i + 1];
            }
        }

        return nullptr;
    };

    const string* leftValue = fieldValue(condition.columnName);
    if (leftValue == nullptr) {
        return false;
    }

    if (!condition.expectedIsColumn) {
        return *leftValue == condition.expectedValue;
    }

    const string* rightValue = fieldValue(condition.expectedValue);
    return rightValue != nullptr && *leftValue == *rightValue;
}

bool SQLcommands::matches(const string& tableName, const Vector<string>& fields, const WhereClause& where) {
    for (size_t i = 0; i < where.groups.Size(); ++i) {
        const Vector<Condition>& group = where.groups[i];
        bool groupMatches = true;

        for (size_t j = 0; j < group.Size(); ++j) {
            if (!matches(tableName, fields, group[j])) {
                groupMatches = false;
                break;
            }
        }

        if (groupMatches) {
            return true;
        }
    }

    return false;
}

bool SQLcommands::select(const string& tableName, const WhereClause* where, const Vector<string>& columns) {
    Vector<size_t> indexes;
    const size_t columnCount = config.countColumns(tableName);

    for (size_t i = 0; i < columns.Size(); i++) {
        if (columns[i] == tableName + "_pk") {
            indexes.pushBack(0);
            continue;
        }

        bool found = false;
        for (size_t j = 0; j < columnCount; ++j) {
            if (config.columnName(tableName, j) == columns[i]) {
                indexes.pushBack(j + 1);
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "Колонка не найдена: " << columns[i] << '\n';
            return false;
        }
    }

    fs::path pathToTable = config.databaseName() / fs::path(tableName);
    FileReader tableReader(pathToTable);
    string line;
    Vector<string> fields;

    while (tableReader.nextRow(line)) {
        tableReader.readLine(line, fields);

        if (where != nullptr && !matches(tableName, fields, *where)) {
            continue;
        }

        for (size_t i = 0; i < indexes.Size(); ++i) {
            if (i > 0) {
                std::cout << ',';
            }
            std::cout << fields[indexes[i]];
        }
        std::cout << '\n';
    }

    if (tableReader.hasReadError()) {
        cerr << "ERROR: unable to read table CSV\n";
        return false;
    }

    return true;
}

bool SQLcommands::select(const Vector<string>& tableNames, const WhereClause* where, 
                         const Vector<string>& columns) {
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

    if (tableNames.Size() == 0 || columns.Size() == 0) {
        return false;
    }

    Vector<fs::path> paths;
    for (size_t i = 0; i < tableNames.Size(); ++i) {
        fs::path path = fs::path(config.databaseName()) / tableNames[i];
        if (!fs::is_regular_file(path / "1.csv")) {
            cerr << "ERROR: table not found: " << tableNames[i] << '\n';
            return false;
        }
        paths.pushBack(path);
    }

    auto findField = [&](const string& name, FieldIndex& index) -> bool {
        size_t dot = name.find('.');
        string table = dot == string::npos ? "" : name.substr(0, dot);
        string column = dot == string::npos ? name : name.substr(dot + 1);
        bool found = false;

        for (size_t i = 0; i < tableNames.Size(); ++i) {
            if (dot != string::npos && tableNames[i] != table) {
                continue;
            }

            size_t field = 0;
            if (column != tableNames[i] + "_pk") {
                size_t count = config.countColumns(tableNames[i]);
                size_t j = 0;
                while (j < count && config.columnName(tableNames[i], j) != column) {
                    ++j;
                }
                if (j == count) {
                    continue;
                }
                field = j + 1;
            }

            if (found) {
                return false;
            }

            index = FieldIndex{i, field};
            found = true;
        }

        return found;
    };

    Vector<FieldIndex> indexes;
    for (size_t i = 0; i < columns.Size(); ++i) {
        FieldIndex index;
        if (!findField(columns[i], index)) {
            cerr << "ERROR: unknown or ambiguous column: " << columns[i] << '\n';
            return false;
        }
        indexes.pushBack(index);
    }

    Vector<Vector<IndexedCondition>> groups;
    if (where != nullptr) {
        if (where->groups.Size() == 0) {
            return false;
        }

        for (size_t g = 0; g < where->groups.Size(); ++g) {
            const Vector<Condition>& group = where->groups[g];
            if (group.Size() == 0) {
                return false;
            }

            Vector<IndexedCondition> indexedGroup;
            for (size_t c = 0; c < group.Size(); ++c) {
                FieldIndex index;
                if (!findField(group[c].columnName, index)) {
                    cerr << "ERROR: unknown or ambiguous column: "
                         << group[c].columnName << '\n';
                    return false;
                }
                FieldIndex rightIndex{0, 0};
                if (group[c].expectedIsColumn &&
                    !findField(group[c].expectedValue, rightIndex)) {
                    cerr << "ERROR: unknown or ambiguous column: "
                         << group[c].expectedValue << '\n';
                    return false;
                }
                indexedGroup.pushBack(IndexedCondition{
                    index, rightIndex, group[c].expectedValue, group[c].expectedIsColumn
                });
            }
            groups.pushBack(indexedGroup);
        }
    }

    Vector<Vector<string>> currentRows;
    for (size_t i = 0; i < tableNames.Size(); ++i) {
        currentRows.pushBack(Vector<string>{});
    }

    auto fieldValue = [&](const FieldIndex& index) -> const string& {
        return currentRows[index.table][index.field];
    };

    auto matchesWhere = [&]() -> bool {
        if (where == nullptr) {
            return true;
        }

        for (size_t g = 0; g < groups.Size(); ++g) {
            bool groupMatches = true;
            for (size_t c = 0; c < groups[g].Size(); ++c) {
                const IndexedCondition& condition = groups[g][c];
                const string& rightValue = condition.expectedIsColumn
                    ? fieldValue(condition.rightIndex)
                    : condition.expectedValue;
                if (fieldValue(condition.index) != rightValue) {
                    groupMatches = false;
                    break;
                }
            }
            if (groupMatches) {
                return true;
            }
        }

        return false;
    };

    std::function<bool(size_t)> scan = [&](size_t level) -> bool {
        if (level == tableNames.Size()) {
            if (matchesWhere()) {
                for (size_t i = 0; i < indexes.Size(); ++i) {
                    if (i > 0) {
                        cout << ',';
                    }
                    cout << fieldValue(indexes[i]);
                }
                cout << '\n';
            }
            return true;
        }

        FileReader tableReader(paths[level]);
        string line;
        while (tableReader.nextRow(line)) {
            tableReader.readLine(line, currentRows[level]);
            if (currentRows[level].Size() != config.countColumns(tableNames[level]) + 1) {
                cerr << "ERROR: invalid CSV row in " << tableNames[level] << '\n';
                return false;
            }
            if (!scan(level + 1)) {
                return false;
            }
        }

        if (tableReader.hasReadError()) {
            cerr << "ERROR: unable to read table CSV\n";
            return false;
        }

        return true;
    };

    return scan(0);
}

bool SQLcommands::deleteRows(const string& tableName, const WhereClause& where) {
    fs::path pathToTable = fs::path(config.databaseName()) / tableName;
    if (!fs::is_directory(pathToTable)) {
        cerr << "ERROR: table not found\n";
        return false;
    }

    if (where.groups.Size() == 0) {
        return false;
    }

    for (size_t g = 0; g < where.groups.Size(); ++g) {
        const Vector<Condition>& group = where.groups[g];
        if (group.Size() == 0) {
            return false;
        }

        for (size_t c = 0; c < group.Size(); ++c) {
            const Condition& condition = group[c];
            for (size_t side = 0; side < (condition.expectedIsColumn ? 2 : 1); ++side) {
                const string& columnName = side == 0
                    ? condition.columnName
                    : condition.expectedValue;
                bool found = columnName == tableName + "_pk";

                for (size_t i = 0; !found && i < config.countColumns(tableName); i++) {
                    if (config.columnName(tableName, i) == columnName) {
                        found = true;
                    }
                }

                if (!found) {
                    cerr << "ERROR: column not found\n";
                    return false;
                }
            }
        }
    }

    TableLock tableLock(pathToTable / (tableName + "_lock"));
    if (!tableLock.isLocked()) {
        cerr << "ERROR: unable to lock table: " << tableName << '\n';
        return false;
    }

    FileReader tableReader(pathToTable);
    Vector<string> fields;
    int fileNumber = 1;

    while (true) {
        string fileName = to_string(fileNumber) + ".csv";
        fs::path csvPath = pathToTable / fileName;
        if (!fs::exists(csvPath)) {
            break;
        }

        ifstream tableFile(csvPath);
        string header;
        if (!getline(tableFile, header)) {
            cerr << "ERROR: unable to read CSV header\n";
            return false;
        }

        fs::path tempPath = pathToTable / (fileName + ".tmp");
        ofstream tempFile(tempPath);
        if (!tempFile.is_open()) {
            cerr << "ERROR: unable to open temporary file\n";
            return false;
        }

        tempFile << header << '\n';

        string line;
        while (getline(tableFile, line)) {
            tableReader.readLine(line, fields);

            if (!matches(tableName, fields, where)) {
                tempFile << line << '\n';
            }
        }

        bool readError = tableFile.bad() || !tableFile.eof();
        tableFile.close();
        tempFile.close();

        if (readError || tempFile.fail()) {
            cerr << "ERROR: unable to copy CSV\n";
            return false;
        }

        std::error_code error;
        fs::rename(tempPath, csvPath, error);
        if (error) {
            cerr << "ERROR: unable to replace CSV\n";
            return false;
        }

        fileNumber++;
    }

    return true;
}
