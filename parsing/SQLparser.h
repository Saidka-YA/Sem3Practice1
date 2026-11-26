#pragma once

#include <cstddef>
#include <string>
#include <cctype>
#include <iostream>
#include "../SQL/SQL.h"
#include "../struct/Vector.hpp"

using namespace std;

class SQLParser {
private:
    enum class TokenKind {
        Word,
        String,
        Symbol
    };

    struct Token {
        TokenKind kind;
        string text;
    };

    SQLcommands& sql;
    Vector<Token> tokens;
    size_t position = 0;

    bool tokenize(const string& input);
    bool takeKeyword(const string& keyword);
    bool takeSymbol(char symbol);
    bool takeText(TokenKind kind, string& text);
    bool readList(TokenKind kind, Vector<string>& values);
    bool readWhere(WhereClause& where);
    bool atEnd();

    bool parseInsert();
    bool parseSelect();
    bool parseDelete();

public:
    explicit SQLParser(SQLcommands& sql) : sql(sql) {}

    bool execute(const string& input);
};
