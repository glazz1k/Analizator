#include "Lexer.h"
#include "HashTable.h"
#include "Parser.h"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "Russian");
    string inputFile = "input.txt";
    string outputFile = "output.txt";

    ofstream output(outputFile);
    HashTable hashTable;
    HashTable declaredVarsTable;

    // ОДИН раз читаем файл и сохраняем все токены
    vector<Token> allTokens;
    {
        Lexer fileLexer(inputFile, &hashTable);
        while (fileLexer.hasMoreTokens())
        {
            Token token = fileLexer.getNextToken();
            if (token.getType() == TokenType::END_OF_FILE)
                break;
            allTokens.push_back(token);
        }
    }

    // Вывод хеш-таблицы
    hashTable.printToFile(output);
    output << "\n";

    // Синтаксический анализ использует токены из памяти
    Lexer memoryLexer(allTokens, &hashTable);
    Parser parser(memoryLexer, output, &declaredVarsTable);
    bool syntaxCorrect = parser.parse();

    output.close();

    cout << "Анализ завершен. Результат в: " << outputFile << endl;
    cout << "Синтаксический анализ: " << (syntaxCorrect ? "УСПЕХ" : "ОШИБКИ") << endl;

    return 0;
}