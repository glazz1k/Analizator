#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "Token.h"
#include <fstream>
#include <vector>

struct HashEntry            // Структура представляет одну запись в хеш-таблице
{
    Token token;            // Хранимый токен (лексема)
    bool occupied;          // Флаг, указывающий занята ли ячейка
    HashEntry* next;        // Указатель на следующую запись в цепочке
    int sequentialIndex;    // Уникальный последовательный индекс для вывода
    string varType;         // Тип переменной

    HashEntry() : occupied(false), next(nullptr), sequentialIndex(-1) {}
    HashEntry(const Token& t) : token(t), occupied(true), next(nullptr), sequentialIndex(-1) {}
    HashEntry(const Token& t, const string& type) : token(t), occupied(true), next(nullptr), sequentialIndex(-1), varType(type) {}
};

class HashTable
{
private:
    static const int TABLE_SIZE = 100;  // Размер хеш-таблицы (фиксированный)
    HashEntry* table[TABLE_SIZE];       // Массив указателей на записи таблицы
    int sequentialIndex;                // Счетчик для последовательной нумерации записей

    int hashFunction(const string& value) const;    // Хеш-функция - преобразует строку в индекс таблицы

public:
    HashTable();
    ~HashTable();

    int insert(const Token& token);                 // Вставка токена в таблицу
    int insertWithType(const Token& token, const string& type); // Вставка с указанием типа
    void printToFile(ofstream& output) const;       // Вывод таблицы в файл
    void clear();                                   // Очистка таблицы

    bool contains(const string& value) const
    {
        int index = hashFunction(value);
        HashEntry* current = table[index];

        while (current != nullptr)
        {
            if (current->occupied && current->token.getValue() == value)
                return true;
            current = current->next;
        }
        return false;
    }

    string getVariableType(const string& varName) const     // Получение типа переменной
    {
        int index = hashFunction(varName);
        HashEntry* current = table[index];

        while (current != nullptr)
        {
            if (current->occupied && current->token.getValue() == varName)
                return current->varType;
            current = current->next;
        }
        return ""; // Пустая строка, если переменная не найдена
    }
};

#endif