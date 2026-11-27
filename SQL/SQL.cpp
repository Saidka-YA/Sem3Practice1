#include "./SQL.h"

bool SQLcommands::insert(const string& tableName, const Vector<string>& values) {
    if (values.Size() != config.countColumns(tableName)) {
        cerr << "ERROR: INSERT into " << tableName << " expects " << config.countColumns(tableName)
             << " values, got " << values.Size() << '\n';
        return false;
    }

    fs::path pathToTable = fs::path(config.databaseName()) / tableName;

    TableLock tableLock(pathToTable / (tableName + "_lock"));
    if (!tableLock.isLocked()) {
        cerr << "ERROR: INSERT cannot lock table: " << tableName << '\n';
        return false;
    }

    fs::path sequencePath = pathToTable / (tableName + "_pk_sequence");
    ifstream readPKSequence(sequencePath);
    if (!readPKSequence.is_open()) {
        cerr << "ERROR: unable to open primary key sequence: " << sequencePath << '\n';
        return false;
    }
    int key; 
    if (!(readPKSequence >> key)) {
        cerr << "ERROR: invalid primary key sequence: " << sequencePath << '\n';
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
                cerr << "ERROR: unable to create CSV file: " << csvPath << '\n';
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
            cerr << "ERROR: unable to open CSV file: " << csvPath << '\n';
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
        cerr << "ERROR: unable to open CSV file for INSERT: " << csvPath << '\n';
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
        cerr << "ERROR: unable to write new row to CSV file: " << csvPath << '\n';
        return false;
    }
    
    tableFile.close();

    if (tableFile.fail()) {
        cerr << "ERROR: unable to close CSV file after INSERT: " << csvPath << '\n';
        return false;
    }
    
    ofstream writePKSequence(pathToTable / (tableName + "_pk_sequence"));
    if (!writePKSequence.is_open()) {
        cerr << "ERROR: unable to open primary key sequence for writing: " << sequencePath << '\n';
        return false;
    }

    writePKSequence << newKey;

    writePKSequence.flush();
    if (!writePKSequence) {
        cerr << "ERROR: unable to write primary key sequence: " << sequencePath << '\n';
        return false;
    }

    writePKSequence.close();

    if (writePKSequence.fail()) {
        cerr << "ERROR: unable to close primary key sequence: " << sequencePath << '\n';
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

bool SQLcommands::findField(const Vector<string>& tableNames, const string& name, FieldIndex& index) {
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
}

const string& SQLcommands::fieldValue(const Vector<Vector<string>>& currentRows, const FieldIndex& index) {
    return currentRows[index.table][index.field];
}

bool SQLcommands::matchesWhere(const Vector<Vector<IndexedCondition>>& groups,
                               const Vector<Vector<string>>& currentRows) {
    if (groups.Size() == 0) {
        return true;
    }

    for (size_t g = 0; g < groups.Size(); ++g) {
        bool groupMatches = true;
        for (size_t c = 0; c < groups[g].Size(); ++c) {
            const IndexedCondition& condition = groups[g][c];
            const string& rightValue = condition.expectedIsColumn
                ? fieldValue(currentRows, condition.rightIndex)
                : condition.expectedValue;
            if (fieldValue(currentRows, condition.index) != rightValue) {
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

bool SQLcommands::scan(size_t level, const Vector<string>& tableNames, const Vector<fs::path>& paths,
                       const Vector<FieldIndex>& indexes, const Vector<Vector<IndexedCondition>>& groups,
                       Vector<Vector<string>>& currentRows) {
    if (level == tableNames.Size()) {
        if (matchesWhere(groups, currentRows)) {
            for (size_t i = 0; i < indexes.Size(); ++i) {
                if (i > 0) {
                    cout << ',';
                }
                cout << fieldValue(currentRows, indexes[i]);
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
            cerr << "ERROR: CSV row has the wrong number of fields in table: " << tableNames[level] << '\n';
            return false;
        }
        if (!scan(level + 1, tableNames, paths, indexes, groups, currentRows)) {
            return false;
        }
    }

    if (tableReader.hasReadError()) {
        cerr << "ERROR: unable to read CSV files for table: " << tableNames[level] << '\n';
        return false;
    }

    return true;
}

bool SQLcommands::select(const Vector<string>& tableNames, const WhereClause* where,
                         const Vector<string>& columns) {
    if (tableNames.Size() == 0 || columns.Size() == 0) {
        return false;
    }

    Vector<fs::path> paths;
    for (size_t i = 0; i < tableNames.Size(); ++i) {
        fs::path path = fs::path(config.databaseName()) / tableNames[i];
        if (!fs::is_regular_file(path / "1.csv")) {
            cerr << "ERROR: SELECT table not found or 1.csv missing: " << tableNames[i] << '\n';
            return false;
        }
        paths.pushBack(path);
    }

    Vector<FieldIndex> indexes;
    for (size_t i = 0; i < columns.Size(); ++i) {
        FieldIndex index;
        if (!findField(tableNames, columns[i], index)) {
            cerr << "ERROR: SELECT column not found or ambiguous: " << columns[i] << '\n';
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
                if (!findField(tableNames, group[c].columnName, index)) {
                    cerr << "ERROR: WHERE column not found or ambiguous: "
                         << group[c].columnName << '\n';
                    return false;
                }
                FieldIndex rightIndex{0, 0};
                if (group[c].expectedIsColumn &&
                    !findField(tableNames, group[c].expectedValue, rightIndex)) {
                    cerr << "ERROR: WHERE right column not found or ambiguous: "
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

    return scan(0, tableNames, paths, indexes, groups, currentRows);
}

bool SQLcommands::deleteRows(const string& tableName, const WhereClause& where) {
    fs::path pathToTable = fs::path(config.databaseName()) / tableName;
    if (!fs::is_directory(pathToTable)) {
        cerr << "ERROR: DELETE table not found: " << tableName << '\n';
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
                    cerr << "ERROR: DELETE column not found in table " << tableName << ": " << columnName << '\n';
                    return false;
                }
            }
        }
    }

    TableLock tableLock(pathToTable / (tableName + "_lock"));
    if (!tableLock.isLocked()) {
        cerr << "ERROR: DELETE cannot lock table: " << tableName << '\n';
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
            cerr << "ERROR: unable to read CSV header: " << csvPath << '\n';
            return false;
        }

        fs::path tempPath = pathToTable / (fileName + ".tmp");
        ofstream tempFile(tempPath);
        if (!tempFile.is_open()) {
            cerr << "ERROR: unable to open temporary CSV file: " << tempPath << '\n';
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
            cerr << "ERROR: unable to read CSV or write temporary file: " << csvPath << '\n';
            return false;
        }

        std::error_code error;
        fs::rename(tempPath, csvPath, error);
        if (error) {
            cerr << "ERROR: unable to replace CSV file " << csvPath << ": " << error.message() << '\n';
            return false;
        }

        fileNumber++;
    }

    return true;
}
