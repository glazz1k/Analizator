#include "Lexer.h"
#include <iostream>

// Конструктор лексера - открывает файл и читает первый символ
Lexer::Lexer(const string& filename, HashTable* ht)
    : hashTable(ht), currentLine(1), currentPos(1), useMemoryMode(false), memoryIndex(0)
{
    input.open(filename);
    if (input.is_open())
        currentChar = input.get();
    else
    {
        currentChar = EOF;
        cout << "Ошибка: не удалось открыть файл " << filename << endl;
    }
}

// Новый конструктор для работы с памятью
Lexer::Lexer(const vector<Token>& tokens, HashTable* ht)
    : hashTable(ht), memoryTokens(tokens), memoryIndex(0), useMemoryMode(true),
    currentLine(1), currentPos(1), currentChar(EOF)
{
    // Ничего не делаем - все токены уже в памяти
}

Lexer::~Lexer()
{
    if (input.is_open())
        input.close();              // Закрываем файловый поток
}

void Lexer::skipWhitespace() // Пропуск пробелов
{
    while (hasMoreTokens())
    {
        if (currentChar == '\n')
        {
            currentLine++;
            currentPos = 1;
            currentChar = input.get();
        }
        else if (isspace(currentChar))
        {
            currentPos++;
            currentChar = input.get();
        }
        else
            break;
    }
}

char Lexer::getNextChar()           // Получает следующий символ из потока
{
    if (input.eof())
        return EOF;                 // Достигнут конец файла

    char ch = input.get();
    if (ch != EOF && ch != '\n')    // Обновляем позицию
        currentPos++;

    return ch;
}

bool Lexer::hasMoreTokens() const   // Проверяет, есть ли еще символы для обработки
{
    if (useMemoryMode) {
        return memoryIndex < memoryTokens.size();
    }
    return currentChar != EOF;
}

Token Lexer::recognizeNumber()
{
    string value;
    int startLine = currentLine;
    int startPos = currentPos;
    bool hasDot = false;
    bool hasError = false;

    // Проверка: если число начинается с 0 и следующий символ - цифра, это ошибка
    if (currentChar == '0' && hasMoreTokens())
    {
        char nextChar = input.peek(); // Смотрим следующий символ без чтения
        if (isdigit(nextChar))
            hasError = true; // Число начинается с 0 и имеет другие цифры - ошибка
    }

    // Сначала собираем все символы, которые могут быть частью числа или ошибочного идентификатора
    while (hasMoreTokens() && (isdigit(currentChar) || currentChar == '.' || isalpha(currentChar) || currentChar == '_'))
    {
        if (isalpha(currentChar) || currentChar == '_')
            hasError = true; // Нашли букву или _ - это ошибка

        if (currentChar == '.')
        {
            if (hasDot)
                hasError = true;  // Уже была точка - ошибка
            hasDot = true;
        }

        value += currentChar;
        currentChar = getNextChar();
    }

    // Если есть ошибка
    if (hasError || value.back() == '.')
        return Token(TokenType::ERROR, value, startLine, startPos);

    if (hasDot)
        return Token(TokenType::DOUBLE_NUM, value, startLine, startPos);
    else
        return Token(TokenType::INT_NUM, value, startLine, startPos);
}

Token Lexer::recognizeIdentifierOrKeyword()
{
    string value;
    int startLine = currentLine;
    int startPos = currentPos;
    bool foundDigits = false;
    bool hasError = false;

    // Собираем первый символ
    value += currentChar;
    currentChar = getNextChar();

    // Продолжаем собирать остальные символы идентификатора
    while (hasMoreTokens() && (isalnum(currentChar) || currentChar == '_'))
    {
        // Если нашли цифру - отмечаем, что началась цифровая часть
        if (isdigit(currentChar))
            foundDigits = true;

        // Если нашли букву после цифр - это ошибка
        if (isalpha(currentChar) && foundDigits)
            hasError = true;

        value += currentChar;
        currentChar = getNextChar();
    }

    // Если есть буквы после цифр - возвращаем ERROR
    if (hasError)
        return Token(TokenType::ERROR, value, startLine, startPos);

    // Проверяем, является ли идентификатор ключевым словом
    if (value == "return") return Token(TokenType::RETURN, value, startLine, startPos);
    if (value == "int") return Token(TokenType::INT, value, startLine, startPos);
    if (value == "double") return Token(TokenType::DOUBLE, value, startLine, startPos);
    if (value == "itod") return Token(TokenType::ITOD, value, startLine, startPos);
    if (value == "dtoi") return Token(TokenType::DTOI, value, startLine, startPos);

    return Token(TokenType::ID, value, startLine, startPos);
}

// Вспомогательная функция для проверки, является ли символ оператором или разделителем
bool Lexer::isOperatorOrDelimiter(char ch) const
{
    return ch == '=' || ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
        ch == ',' || ch == ';' || ch == '(' || ch == ')' ||
        ch == '{' || ch == '}' || ch == EOF;
}

Token Lexer::recognizeErrorIdentifier()
{
    string value;
    int startLine = currentLine;
    int startPos = currentPos;

    // Собираем символы ошибочного идентификатора
    value += currentChar; // первый символ
    currentChar = getNextChar();

    // Продолжаем собирать, пока идут любые символы кроме пробелов и известных операторов
    while (hasMoreTokens() && !isspace(currentChar) && !isOperatorOrDelimiter(currentChar))
    {
        value += currentChar;
        currentChar = getNextChar();
    }

    return Token(TokenType::ERROR, value, startLine, startPos);
}

Token Lexer::recognizeOperator()
{
    int startLine = currentLine;
    int startPos = currentPos;
    char ch = currentChar;
    currentChar = getNextChar();

    switch (ch) // Определяем тип оператора по символу
    {
    case '=': return Token(TokenType::ASSIGN, "=", startLine, startPos);
    case '+': return Token(TokenType::PLUS, "+", startLine, startPos);
    case '-': return Token(TokenType::MINUS, "-", startLine, startPos);
    case '*': return Token(TokenType::MULT, "*", startLine, startPos);
    case '/': return Token(TokenType::DIV, "/", startLine, startPos);
    case ',': return Token(TokenType::COMMA, ",", startLine, startPos);
    case ';': return Token(TokenType::SEMICOLON, ";", startLine, startPos);
    case '(': return Token(TokenType::LPAREN, "(", startLine, startPos);
    case ')': return Token(TokenType::RPAREN, ")", startLine, startPos);
    case '{': return Token(TokenType::LBRACE, "{", startLine, startPos);
    case '}': return Token(TokenType::RBRACE, "}", startLine, startPos);
    case EOF: return Token(TokenType::END_OF_FILE, "", startLine, startPos);
    default:
        return Token(TokenType::ERROR, std::string(1, ch), startLine, startPos); // Неизвестный символ - ошибка
    }
}

Token Lexer::getNextToken()
{
    if (useMemoryMode) {
        // Режим памяти - берем токены из вектора
        if (memoryIndex < memoryTokens.size()) {
            return memoryTokens[memoryIndex++];
        }
        return Token(TokenType::END_OF_FILE, "", 0, 0);
    }

    skipWhitespace();

    if (!hasMoreTokens())
        return Token(TokenType::END_OF_FILE, "", currentLine, currentPos);

    Token token;

    if (isdigit(currentChar))
        token = recognizeNumber();              // Число
    else if (isalpha(currentChar))
        token = recognizeIdentifierOrKeyword(); // Идентификатор или ключевое слово
    else if (currentChar == '_' || !isOperatorOrDelimiter(currentChar))
        token = recognizeErrorIdentifier();     // Ошибочный идентификатор
    else
        token = recognizeOperator();            // Оператор или разделитель

    if (token.getType() != TokenType::END_OF_FILE)
        hashTable->insert(token);

    return token;
}

Token Lexer::peekNextToken()
{
    if (useMemoryMode) {
        // Режим памяти
        if (memoryIndex < memoryTokens.size()) {
            return memoryTokens[memoryIndex];
        }
        return Token(TokenType::END_OF_FILE, "", 0, 0);
    }
    // Сохраняем текущее состояние
    int oldLine = currentLine;
    int oldPos = currentPos;
    char oldCurrentChar = currentChar;
    streampos oldFilePos = input.tellg();
    // streampos - специальный тип данных в C++ для хранения позиции в потоке
    // tellg() - "tell get position" - возвращает текущую позицию чтения в потоке

    // Получаем следующий токен
    Token nextToken = getNextToken();

    // Восстанавливаем состояние
    currentLine = oldLine;
    currentPos = oldPos;
    currentChar = oldCurrentChar;
    input.seekg(oldFilePos);
    // seekg() - "seek get position" - перемещает указатель чтения на заданную позицию

    return nextToken;
}