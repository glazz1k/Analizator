#include "Token.h"
#include <map>

Token::Token() : type(TokenType::ERROR), value(""), line(0), position(0) {}

Token::Token(TokenType t, const string& v, int l, int p) : type(t), value(v), line(l), position(p) {}

TokenType Token::getType() const
{
    return type;
}

string Token::getValue() const
{
    return value;
}

int Token::getLine() const
{
    return line;
}

int Token::getPosition() const
{
    return position;
}

string Token::getTypeString() const
{
    static map<TokenType, string> typeStrings =
    {
        // «аполн€ем соответстви€ тип -> строка
        {TokenType::RETURN, "RETURN"},
        {TokenType::INT, "INT"},
        {TokenType::DOUBLE, "DOUBLE"},
        {TokenType::ITOD, "ITOD"},
        {TokenType::DTOI, "DTOI"},
        {TokenType::ID, "ID"},
        {TokenType::INT_NUM, "INT_NUM"},
        {TokenType::DOUBLE_NUM, "DOUBLE_NUM"},
        {TokenType::ASSIGN, "ASSIGN"},
        {TokenType::PLUS, "PLUS"},
        {TokenType::MINUS, "MINUS"},
        {TokenType::MULT, "MULT"},
        {TokenType::DIV, "DIV"},
        {TokenType::COMMA, "COMMA"},
        {TokenType::SEMICOLON, "SEMICOLON"},
        {TokenType::LPAREN, "LPAREN"},
        {TokenType::RPAREN, "RPAREN"},
        {TokenType::LBRACE, "LBRACE"},
        {TokenType::RBRACE, "RBRACE"},
        {TokenType::END_OF_FILE, "END_OF_FILE"},
        {TokenType::ERROR, "ERROR"}
    };

    return typeStrings[type]; // ¬озвращаем строковое представление типа
}