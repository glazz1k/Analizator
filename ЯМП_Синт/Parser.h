#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "HashTable.h"
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <stack>

using namespace std;

class Parser {
private:
    Lexer& lexer;
    ofstream& output;
    Token currentToken;
    Token lastProcessedToken;
    Token lastValidToken; 
    vector<string> errors;
    HashTable* declaredVariables;

    void advance();
    bool match(TokenType expectedType, const string& errorMsg);
    void error(const string& message);

    void function();
    void descriptions();
    void operators();
    void descr();
    void varlist(const string& varType = "");
    void type();
    void op();
    void expr(int indentLevel = 0);
    void simpleExpr(int indentLevel = 0);
    void skipToSemicolon();
    void skipToSemicolonOrBrace();
    void showErroneousDescription();
    void processVariableListForUnknownType();
    bool begin();
    bool end();

    Token peekNextToken() { return lexer.peekNextToken(); }

    void addDeclaredVariable(const string& name); // Добавляем переменную в таблицу объявленных переменных.
    bool isVariableDeclared(const string& name); // Проверяем, была ли переменная объявлена ранее.
    void clearDeclaredVariables();

    // Поля для семантического анализа
    string currentFunctionType;         // Тип текущей функции
    string currentFunctionName;         // Имя текущей функции
    vector<string> postfixCode;         // Постфиксная запись
    vector<string> currentExpression;   // Текущее выражение для постфикса

public:
    Parser(Lexer& l, ofstream& out, HashTable* varsTable);
    bool parse();

    // Методы для семантического анализа
    void addDeclaredVariableWithType(const string& varName, const string& type);
    void generatePostfix();
    string getExpressionType();
    void checkAssignmentType(const string& varName, const string& exprType);
    void checkFunctionReturnType(const string& returnVar);
    void processFunctionCall(const string& funcName);
    void completeExpression();
    void checkFunctionArgumentType(const string& funcName, const string& argType);
    void checkBinaryOperationTypes(const string& leftType, const string& rightType, const string& op);
    void addToPostfix(const string& token);

    string getVariableType(const string& varName) const
    {
        return declaredVariables->getVariableType(varName);
    }
};

#endif