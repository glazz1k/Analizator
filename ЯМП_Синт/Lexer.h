#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include "HashTable.h"
#include <vector>

class Lexer
{
private:
    ifstream input;
    int currentLine;    // Текущий номер строки
    int currentPos;     // Текущая позиция в строке
    char currentChar;   // Текущий обрабатываемый символ
    HashTable* hashTable;       // Указатель на хеш-таблицу для записи токенов

    vector<Token> memoryTokens;
    size_t memoryIndex;
    bool useMemoryMode;

    void skipWhitespace();      // Пропуск пробельных символов
    char getNextChar();         // Получение следующего символа
    Token recognizeNumber();    // Распознавание чисел
    Token recognizeIdentifierOrKeyword();   // Распознавание идентификаторов и ключевых слов
    Token recognizeOperator();  // Распознавание операторов
    Token recognizeErrorIdentifier(); // Распознавание ошибок
    bool isOperatorOrDelimiter(char c) const;

public:
    Lexer(const string& filename, HashTable* ht);
    Lexer(const vector<Token>& tokens, HashTable* ht);
    ~Lexer();

    Token getNextToken();       // Основной метод - получение следующего токена
    Token peekNextToken();      // Просмотр следующего токена без продвижения
    bool hasMoreTokens() const; // Проверка наличия еще токенов
};

#endif