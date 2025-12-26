#include "HashTable.h"
#include <iostream>
#include <iomanip>

HashTable::HashTable() : sequentialIndex(0)
{
    for (int i = 0; i < TABLE_SIZE; ++i)
        table[i] = nullptr;
}

HashTable::~HashTable()
{
    clear();
}

int HashTable::hashFunction(const string& value) const // Хеш-функция - преобразует строку в индекс от 0 до TABLE_SIZE-1
{
    int hash = 0;               // Начальное значение хеша
    for (char c : value)        // Проходим по всем символам строки
        hash = (hash * 31 + c) % TABLE_SIZE;            // Вычисляем хеш, используя хеш функцию
    return hash;                // Возвращаем индекс в диапазоне [0, TABLE_SIZE-1]
}

int HashTable::insert(const Token& token)
{
    string value = token.getValue();
    int index = hashFunction(value);        // Вычисляем хеш-индекс для значения

    // Проверяем, существует ли уже такая лексема в таблице
    HashEntry* current = table[index];      // Начинаем с начала цепочки ячейки index
    while (current != nullptr)              // Проходим по всей цепочке только этой ячейки
    {
        if (current->occupied && current->token.getValue() == value)
            return current->sequentialIndex;    // Если нашли - возвращаем существующий индекс
        current = current->next;                // Переходим к следующему элементу цепочки
    }

    // Лексема не найдена - создаем новую запись
    HashEntry* newEntry = new HashEntry(token);
    newEntry->sequentialIndex = sequentialIndex++;

    // Вставляем запись в таблицу (разрешение коллизий методом цепочек)
    if (table[index] == nullptr)        // Если ячейка пуста
        table[index] = newEntry;        // Просто помещаем запись в ячейку
    else
    {
        // Ячейка занята - добавляем в конец цепочки
        HashEntry* current = table[index];
        while (current->next != nullptr)        // Находим последний элемент цепочки
            current = current->next;
        current->next = newEntry;       // Добавляем новую запись в конец
    }

    return newEntry->sequentialIndex;   // Возвращаем индекс новой записи
}

int HashTable::insertWithType(const Token& token, const string& type)
{
    string value = token.getValue();
    int index = hashFunction(value);

    // Проверяем, существует ли уже такая лексема в таблице
    HashEntry* current = table[index];
    while (current != nullptr)
    {
        if (current->occupied && current->token.getValue() == value)
        {
            current->varType = type; // Обновляем тип, если запись уже существует
            return current->sequentialIndex;
        }
        current = current->next;
    }

    // Лексема не найдена - создаем новую запись с типом
    HashEntry* newEntry = new HashEntry(token, type);
    newEntry->sequentialIndex = sequentialIndex++;

    // Вставляем запись в таблицу
    if (table[index] == nullptr)
        table[index] = newEntry;
    else
    {
        HashEntry* current = table[index];
        while (current->next != nullptr)
            current = current->next;
        current->next = newEntry;
    }

    return newEntry->sequentialIndex;
}

void HashTable::printToFile(ofstream& output) const
{
    output << setw(35) << "=== ХЕШ-ТАБЛИЦА ===" << "\n";
    output << left;
    output << setw(15) << "Тип лексемы" << " | "
        << setw(15) << "Лексема" << " | "
        << "Индекс\n";
    output << "----------------|-----------------|-------\n";

    HashEntry** entries = new HashEntry*[sequentialIndex]; // Массив указателей на записи
    for (int i = 0; i < sequentialIndex; ++i)
        entries[i] = nullptr;       // Инициализируем все элементы как nullptr

    // Собираем все записи
    for (int i = 0; i < TABLE_SIZE; ++i)    // Проходим по всем ячейкам таблицы
    {
        HashEntry* current = table[i];      // Начинаем с начала цепочки
        while (current != nullptr)          // Проходим по всей цепочке
        {
            if (current->occupied)          // Если запись занята
                entries[current->sequentialIndex] = current;    // Помещаем в массив по sequentialIndex
            current = current->next;        // Переходим к следующему элементу цепочки
        }
    }

    // Выводим записи в порядке возрастания sequentialIndex
    for (int i = 0; i < sequentialIndex; ++i)
    {
        if (entries[i] != nullptr) 
        {
            output << setw(15) << entries[i]->token.getTypeString() << " | "
                << setw(15) << entries[i]->token.getValue() << " | "
                << i << "\n";
        }
    }

    delete[] entries;
    output << "\nВсего уникальных лексем: " << sequentialIndex << "\n";
}

void HashTable::clear()
{
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        HashEntry* current = table[i];
        while (current != nullptr)
        {
            HashEntry* next = current->next;    // Сохраняем указатель на следующий элемент
            delete current;                     // Удаляем текущий элемент
            current = next;                     // Переходим к следующему элементу
        }
        table[i] = nullptr; // Обнуляем указатель ячейки
    }
}