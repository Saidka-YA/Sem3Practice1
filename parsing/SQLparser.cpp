#include "./SQLparser.h"

bool SQLParser::tokenize(const string& input) {
    tokens.Clear();
    position = 0;

    const string symbols = "(),=;";
    size_t i = 0;

    while (i < input.size()) {
        char ch = input[i];

        if (isspace(static_cast<unsigned char>(ch))) {
            i++;
            continue;
        }

        if (symbols.find(ch) != string::npos) {
            tokens.pushBack(Token{TokenKind::Symbol, string(1, ch)});
            i++;
            continue;
        }

        if (ch == '\'') {
            string value;
            bool closed = false;
            i++;

            while (i < input.size()) {
                if (input[i] != '\'') {
                    value += input[i];
                    i++;
                } else if (i + 1 < input.size() && input[i + 1] == '\'') {
                    value += '\'';
                    i += 2;
                } else {
                    i++;
                    closed = true;
                    break;
                }
            }

            if (!closed) {
                return false;
            }

            tokens.pushBack(Token{TokenKind::String, value});
            continue;
        }

        size_t start = i;
        while (i < input.size() &&
               !isspace(static_cast<unsigned char>(input[i])) &&
               symbols.find(input[i]) == string::npos &&
               input[i] != '\'') {
            i++;
        }

        tokens.pushBack(Token{TokenKind::Word, input.substr(start, i - start)});
    }

    return true;
}

bool SQLParser::takeKeyword(const string& keyword) {
    if (position >= tokens.Size() || tokens[position].kind != TokenKind::Word) {
        return false;
    }

    const string& text = tokens[position].text;
    if (text.size() != keyword.size()) {
        return false;
    }

    for (size_t i = 0; i < text.size(); i++) {
        if (toupper(static_cast<unsigned char>(text[i])) != keyword[i]) {
            return false;
        }
    }

    position++;
    return true;
}

bool SQLParser::takeSymbol(char symbol) {
    if (position >= tokens.Size() ||
        tokens[position].kind != TokenKind::Symbol ||
        tokens[position].text[0] != symbol) {
        return false;
    }

    position++;
    return true;
}

bool SQLParser::takeText(TokenKind kind, string& text) {
    if (position >= tokens.Size() || tokens[position].kind != kind) {
        return false;
    }

    text = tokens[position].text;
    position++;
    return true;
}

bool SQLParser::readList(TokenKind kind, Vector<string>& values) {
    do {
        string value;
        if (!takeText(kind, value)) {
            return false;
        }

        values.pushBack(value);
    } while (takeSymbol(','));

    return true;
}

bool SQLParser::readWhere(WhereClause& where) {
    do {
        Vector<Condition> group;

        do {
            Condition condition;
            if (!takeText(TokenKind::Word, condition.columnName) ||
                !takeSymbol('=')) {
                return false;
            }

            if (takeText(TokenKind::String, condition.expectedValue)) {
                condition.expectedIsColumn = false;
            } else if (takeText(TokenKind::Word, condition.expectedValue)) {
                condition.expectedIsColumn = true;
            } else {
                return false;
            }

            group.pushBack(condition);
        } while (takeKeyword("AND"));

        where.groups.pushBack(group);
    } while (takeKeyword("OR"));

    return true;
}

bool SQLParser::atEnd() {
    takeSymbol(';');
    return position == tokens.Size();
}

bool SQLParser::parseInsert() {
    string tableName;
    Vector<string> values;

    if (!takeKeyword("INTO") || !takeText(TokenKind::Word, tableName) || !takeKeyword("VALUES") ||
        !takeSymbol('(') || !readList(TokenKind::String, values) || !takeSymbol(')') || !atEnd()) {
        cerr << "ERROR: invalid INSERT syntax\n";
        return false;
    }

    for (size_t i = 0; i < values.Size(); i++) {
        if (values[i].find_first_of("\r\n") != string::npos) {
            cerr << "ERROR: INSERT value contains newline\n";
            return false;
        }
    }

    return sql.insert(tableName, values);
}

bool SQLParser::parseSelect() {
    Vector<string> columns;
    Vector<string> tables;
    WhereClause where;

    if (!readList(TokenKind::Word, columns) || !takeKeyword("FROM") || !readList(TokenKind::Word, tables)) {
        cerr << "ERROR: invalid SELECT syntax\n";
        return false;
    }

    bool hasWhere = takeKeyword("WHERE");
    if ((hasWhere && !readWhere(where)) || !atEnd()) {
        cerr << "ERROR: invalid SELECT syntax\n";
        return false;
    }

    return sql.select(tables, hasWhere ? &where : nullptr, columns);
}

bool SQLParser::parseDelete() {
    string tableName;
    WhereClause where;

    if (!takeKeyword("FROM") || !takeText(TokenKind::Word, tableName) || !takeKeyword("WHERE") ||
        !readWhere(where) || !atEnd()) {
        cerr << "ERROR: invalid DELETE syntax\n";
        return false;
    }

    const string prefix = tableName + ".";
    for (size_t i = 0; i < where.groups.Size(); i++) {
        for (size_t j = 0; j < where.groups[i].Size(); j++) {
            string& column = where.groups[i][j].columnName;
            if (column.compare(0, prefix.size(), prefix) == 0) {
                column.erase(0, prefix.size());
            }
            if (where.groups[i][j].expectedIsColumn) {
                string& rightColumn = where.groups[i][j].expectedValue;
                if (rightColumn.compare(0, prefix.size(), prefix) == 0) {
                    rightColumn.erase(0, prefix.size());
                }
            }
        }
    }

    return sql.deleteRows(tableName, where);
}

bool SQLParser::execute(const string& input) {
    if (!tokenize(input)) {
        cerr << "ERROR: unclosed string literal\n";
        return false;
    }

    if (takeKeyword("INSERT")) {
        return parseInsert();
    }
    if (takeKeyword("SELECT")) {
        return parseSelect();
    }
    if (takeKeyword("DELETE")) {
        return parseDelete();
    }

    cerr << "ERROR: unknown command\n";
    return false;
}
