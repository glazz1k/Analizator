#ifndef TOKEN_H
#define TOKEN_H

#include <string>

using namespace std;

enum class TokenType            // Всевозможные типы лексем
{
    RETURN, INT, DOUBLE, ITOD, DTOI,  // Ключевые слова

    ID, INT_NUM, DOUBLE_NUM,    // Идентификаторы и константы

    ASSIGN, PLUS, MINUS, MULT, DIV, COMMA, SEMICOLON, LPAREN, RPAREN, LBRACE, RBRACE, // Операторы и разделители

    END_OF_FILE, ERROR          // Служебные
};

class Token
{
private:
    TokenType type;     // Тип лексемы
    string value;       // Значение лексемы
    int line;           // Номер строки в исходном коде
    int position;       // Позиция в строке

public:
    Token();
    Token(TokenType t, const string& v, int l, int p);

    TokenType getType() const;
    string getValue() const;
    int getLine() const;
    int getPosition() const;
    string getTypeString() const; // Получение строкового представления типа
};

#endif