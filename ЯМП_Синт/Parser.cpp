#include "Parser.h"
#include <iostream>

Parser::Parser(Lexer& l, ofstream& out, HashTable* varsTable)
    : lexer(l),
    output(out),
    declaredVariables(varsTable),
    lastValidToken(TokenType::END_OF_FILE, "", 1, 1), 
    currentToken(TokenType::END_OF_FILE, "", 1, 1),  
    lastProcessedToken(TokenType::END_OF_FILE, "", 1, 1) 
{
    advance();
}

void Parser::advance()  // Переход к следующему токену
{
    if (currentToken.getType() != TokenType::END_OF_FILE)
        lastValidToken = currentToken;  // Сохраняем текущий токен как последний валидный, если он не END_OF_FILE
    currentToken = lexer.getNextToken();
}

bool Parser::match(TokenType expectedType, const string& errorMsg)  // Проверка соответствия текущего токена ожидаемому типу
{
    if (currentToken.getType() == expectedType)
    {
        advance();
        return true;
    }
    else
    {
        error(errorMsg);
        return false;
    }
}

void Parser::error(const string& message)   // Добавление ошибки в список ошибок
{
    string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
        to_string(currentToken.getPosition()) + ": " + message;
    errors.push_back(errorMsg);
}

bool Parser::parse()
{
    try
    {
        clearDeclaredVariables();
        postfixCode.clear();
        currentFunctionType.clear();
        currentFunctionName.clear();

        output << "=== ДЕРЕВО РАЗБОРА ===" << endl;
        function(); // Начинаем разбор с функции

        generatePostfix();  // Генерация и вывод постфиксной записи

        if (currentToken.getType() != TokenType::END_OF_FILE && errors.empty()) // Проверяем, что достигнут конец файла и нет ошибок
            error("ожидался конец файла");
    }
    catch (...)
    {
        error("критическая ошибка во время разбора");
    }

    if (!errors.empty())    // Вывод всех найденных ошибок
    {
        output << endl << "=== ОШИБКИ ===" << endl;
        for (const auto& err : errors)
            output << err << endl;
    }

    output << endl << "=== РЕЗУЛЬТАТ АНАЛИЗА ===" << endl;
    if (errors.empty())
        output << "Программа корректна!" << endl;
    else
        output << "Найдено ошибок: " << errors.size() << endl;

    return errors.empty();
}

// Function → Begin Descriptions Operators End
void Parser::function()
{
    output << "Function" << endl;

    // Begin → Type FunctionName() {
    output << "  Begin" << endl;
    if (!begin())
        return;

    // Descriptions → Descr | Descr Descriptions
    output << "  Descriptions" << endl;
    descriptions();

    // Operators → Op | Op Operators
    output << "  Operators" << endl;
    operators();

    if (currentToken.getType() == TokenType::RBRACE)    // Если есть закрывающая скобка, но нет return - это ошибка
    {
        output << "  End" << endl;
        output << "    return <отсутствует>" << endl;
        output << "    Id: <отсутствует>" << endl;
        output << "    ; <отсутствует>" << endl;
        output << "    }" << endl;
        
        string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
            to_string(currentToken.getPosition()) + ": ожидался return";
        errors.push_back(errorMsg);
        
        advance(); // пропускаем }
    }
    else
    {
        // End → return Id ; }
        output << "  End" << endl;
        end();
    }
}

// Begin → Type FunctionName() {
bool Parser::begin()
{
    // Type
    output << "    Type: ";
    if (currentToken.getType() != TokenType::INT && currentToken.getType() != TokenType::DOUBLE)
    {
        output << "<некорректный тип '" << currentToken.getValue() << "'>" << endl;
        error("некорректный тип функции '" + currentToken.getValue() + "', ожидался int или double");
        advance(); // пропускаем некорректный тип

        output << "    FunctionName: ";
        if (currentToken.getType() == TokenType::ID)
        {
            output << currentToken.getValue() << endl;
            match(TokenType::ID, "ожидалось имя функции");
        }
        else
            output << "<ожидается идентификатор>" << endl;
    }
    else
    {
        string typeName = (currentToken.getType() == TokenType::INT) ? "int" : "double";
        output << typeName << endl;
        currentFunctionType = typeName;
        advance();

        // FunctionName → Id
        output << "    FunctionName: ";
        if (currentToken.getType() == TokenType::ID)
        {
            output << currentToken.getValue() << endl;
            currentFunctionName = currentToken.getValue();  // Сохраняем имя функции
            match(TokenType::ID, "ожидалось имя функции");
        }
        else
        {
            output << "<ожидается идентификатор>" << endl;
            error("ожидалось имя функции");
        }
    }

    // Обработка открывающейся скобки
    output << "    (";
    if (currentToken.getType() == TokenType::LPAREN)
    {
        output << endl;
        match(TokenType::LPAREN, "ожидалась (");
    }
    else
    {
        output << " <отсутствует>" << endl;
        error("ожидалась (");
    }

    // Обработка закрывающейся скобки
    output << "    )";
    if (currentToken.getType() == TokenType::RPAREN)
    {
        output << endl;
        match(TokenType::RPAREN, "ожидалась )");
    }
    else
    {
        output << " <отсутствует>" << endl;
        error("ожидалась )");
    }

    // Обработка открывающейся фигурной скобки
    output << "    {";
    if (currentToken.getType() == TokenType::LBRACE)
    {
        output << endl;
        match(TokenType::LBRACE, "ожидалась {");
    }
    else
    {
        output << " <отсутствует>" << endl;
        error("ожидалась {");
    }

    return true; // Всегда true, чтобы продолжить разбор
}

// End → return Id ; }
bool Parser::end()
{
    output << "    return" << endl;
    if (!match(TokenType::RETURN, "ожидался return"))   // Проверяем наличие ключевого слова return
    {
        output << "    Id: <ожидается идентификатор>" << endl;
        output << "    ;<ожидалась ;>" << endl;
        output << "    }<ожидалась }>" << endl;
        return false;
    }

    output << "    Id: ";
    if (currentToken.getType() == TokenType::ID)    // Проверяем тип текущего токена
    {
        string returnVar = currentToken.getValue();
        output << returnVar << endl;
        checkFunctionReturnType(returnVar); // Проверяем, объявлена ли переменная returnVar

        Token idToken = currentToken;   // Сохраняем токен идентификатора ДО проверки

        if (!match(TokenType::ID, "ожидался идентификатор после return"))   // Проверяем и пропускаем идентификатор
        {
            output << "    ;<ожидалась ;>" << endl;
            output << "    }<ожидалась }>" << endl;
            return false;
        }

        addToPostfix(returnVar);    // Добавляем переменную в постфиксную запись
        addToPostfix("RETURN");     // Добавляем операцию RETURN

        output << "    ;";
        if (currentToken.getType() == TokenType::SEMICOLON) // Проверяем наличие точки с запятой
        {
            output << endl;
            advance();
        }
        else
        {
            output << " <отсутствует>" << endl;
            int errorLine = idToken.getLine();  // Вычисляем позицию для ошибки после идентификатора
            int errorPosition = idToken.getPosition() + idToken.getValue().length();
            string errorMsg = "строка " + to_string(errorLine) +
                ", позиция " + to_string(errorPosition) + ": ожидалась ;";
            errors.push_back(errorMsg);
        }
    }
    else if (currentToken.getType() == TokenType::SEMICOLON)
    {
        // Если после return сразу точка с запятой
        output << "<отсутствует>" << endl;
        string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
            to_string(currentToken.getPosition()) + ": ожидался идентификатор после return";
        errors.push_back(errorMsg);

        output << "    ;" << endl;
        advance();
    }
    else
    {
        // Если нет ни идентификатора, ни точки с запятой
        output << "<отсутствует>" << endl;
        string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
            to_string(currentToken.getPosition()) + ": ожидался идентификатор после return";
        errors.push_back(errorMsg);

        skipToSemicolonOrBrace();   // Пропускаем до точки с запятой или закрывающей скобки
        if (currentToken.getType() == TokenType::SEMICOLON) // Если нашли точку с запятой, выводим ее в дерево
        {
            output << "    ;" << endl;
            advance();
        }
    }

    output << "    }";

    // Сохраняем последний обработанный токен перед проверкой закрывающей скобки
    Token lastTokenBeforeBrace = currentToken;

    if (currentToken.getType() == TokenType::RBRACE)    // Проверяем наличие закрывающейся фигурной скобки
    {
        output << endl;
        advance();
    }
    else
    {
        output << " <отсутствует>" << endl;
        int errorLine, errorPosition;   // Вычисляем позицию для ошибки закрывающей скобки

        // Используем lastValidToken для получения корректных координат
        errorLine = lastValidToken.getLine();
        errorPosition = lastValidToken.getPosition() + lastValidToken.getValue().length();

        string errorMsg = "строка " + to_string(errorLine) +
            ", позиция " + to_string(errorPosition) + ": ожидалась }";
        errors.push_back(errorMsg);
    }

    return true;
}

void Parser::skipToSemicolonOrBrace()   // Пропуск токенов до точки с запятой или закрывающейся фигурной скобки
{
    while (currentToken.getType() != TokenType::SEMICOLON &&
        currentToken.getType() != TokenType::RBRACE &&
        currentToken.getType() != TokenType::END_OF_FILE)
    {
        advance();
    }

    // Если нашли точку с запятой, пропускаем ее, это позволяет продолжить разбор со следующего оператора
    if (currentToken.getType() == TokenType::SEMICOLON)
        advance();
}

// Descriptions → Descr | Descr Descriptions
void Parser::descriptions()
{
    // Обрабатываем все объявления - как с типами, так и без типов
    while (currentToken.getType() == TokenType::INT ||
        currentToken.getType() == TokenType::DOUBLE ||
        (currentToken.getType() == TokenType::ID &&
            (lexer.peekNextToken().getType() == TokenType::COMMA ||
                lexer.peekNextToken().getType() == TokenType::SEMICOLON)))
    {
        if (currentToken.getType() == TokenType::ID)    // Обработка объявлений без типа
        {
            output << "    Descr" << endl;
            output << "      Type: <отсутствует>" << endl;

            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": ожидался тип (int или double) перед '" +
                currentToken.getValue() + "'";
            errors.push_back(errorMsg);

            output << "      VarList" << endl;

            output << "        Id: " << currentToken.getValue() << endl; // Обрабатываем первый идентификатор
            addDeclaredVariable(currentToken.getValue());   // Добавляем переменную без типа в список переменных
            advance(); // Пропускаем идентификатор

            int initialLine = currentToken.getLine();

            // Обработка дополнительных переменных через запятую
            while (currentToken.getType() == TokenType::COMMA ||
                (currentToken.getType() == TokenType::ID && currentToken.getLine() == initialLine))
            {
                if (currentToken.getType() == TokenType::COMMA)
                {
                    output << "        ," << endl;
                    match(TokenType::COMMA, "ожидалась ,");

                    output << "        Id: " << (currentToken.getType() == TokenType::ID ? currentToken.getValue() : "<ожидается идентификатор>") << endl;

                    if (currentToken.getType() == TokenType::ID)
                    {
                        // Добавляем ошибку для каждой переменной без типа
                        string varErrorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                            to_string(currentToken.getPosition()) + ": ожидался тип (int или double) перед '" +
                            currentToken.getValue() + "'";
                        errors.push_back(varErrorMsg);

                        addDeclaredVariable(currentToken.getValue());   // Добавляем переменную
                        advance();
                    }
                }
                else if (currentToken.getType() == TokenType::ID)
                {
                    // Обработка идентификатора без запятой
                    output << "        , <отсутствует>" << endl;
                    output << "        Id: " << currentToken.getValue() << endl;

                    string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                        to_string(currentToken.getPosition()) + ": отсутствует ',' между переменными";
                    errors.push_back(errorMsg);

                    string typeErrorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                        to_string(currentToken.getPosition()) + ": ожидался тип (int или double) перед '" +
                        currentToken.getValue() + "'";
                    errors.push_back(typeErrorMsg);

                    addDeclaredVariable(currentToken.getValue());   // Добавляем переменную
                    advance();
                }
            }

            output << "      ;" << endl;
            if (currentToken.getType() == TokenType::SEMICOLON)
                advance();
            else
                skipToSemicolon();
        }
        else
            descr();    // Обычные объявления с типом
    }

    if (currentToken.getType() == TokenType::ID)  // Обработка случая с неизвестным типом
    {
        Token nextToken = lexer.peekNextToken();    // Заглядываем вперед на следующий токен, чтобы определить контекст
        if (nextToken.getType() == TokenType::ID)   // Если следующий токен - ID, это объявление с неизвестным типом
        {
            output << "    Descr" << endl;
            output << "      Type: <неизвестный тип '" << currentToken.getValue() << "'>" << endl;
            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": неизвестный тип '" +
                currentToken.getValue() + "'";
            errors.push_back(errorMsg);

            advance(); // пропускаем неизвестный тип
            output << "      VarList" << endl;
            processVariableListForUnknownType();

            output << "      ;" << endl;
            if (currentToken.getType() == TokenType::SEMICOLON) // Если точка с запятой
                advance();  // Пропускаем точку с запятой
            else
                skipToSemicolon();  // Пропускаем до точки с запятой
        }
    }
}

// Descr → Type VarList ;
void Parser::descr()
{
    output << "    Descr" << endl;
    output << "      Type: ";

    string currentType;
    if (currentToken.getType() == TokenType::INT || currentToken.getType() == TokenType::DOUBLE) // Тип
    {
        string typeName = (currentToken.getType() == TokenType::INT) ? "int" : "double";
        output << typeName << endl;
        currentType = typeName;

        addToPostfix("DECLARE");
        addToPostfix(typeName);
    }
    else
    {
        output << "<ожидается тип>" << endl;
        currentType = ""; // Пустой тип при ошибке
    }

    type();     // Вызываем метод разбора типа (проверяет и пропускает токен типа)

    output << "      VarList" << endl;
    lastProcessedToken = currentToken;  // Сохраняем последний обработанный токен
    varlist(currentType);  // Разбираем список переменных

    output << "      ;";
    if (currentToken.getType() == TokenType::SEMICOLON) // Проверяем наличие точки с запятой
    {
        output << endl;
        advance();
    }
    else
    {
        output << " <ожидалась ;>" << endl;

        // Вычисляем позицию после последнего идентификатора в списке переменных
        int errorLine = lastProcessedToken.getLine();
        int errorPosition = lastProcessedToken.getPosition() + lastProcessedToken.getValue().length();
        string errorMsg = "строка " + to_string(errorLine) +
            ", позиция " + to_string(errorPosition) + ": ожидалась ;";
        errors.push_back(errorMsg);
    }
}

// Type → int | double
void Parser::type()
{
    if (currentToken.getType() == TokenType::INT)           // Если токен int
        match(TokenType::INT, "ожидался int");              // Проверяем и пропускаем
    else if (currentToken.getType() == TokenType::DOUBLE)   // Если токен double
        match(TokenType::DOUBLE, "ожидался double");        // Проверяем и пропускаем
    else
        error("ожидался тип (int или double)");             // Добавляем ошибку
}

// VarList → Id | Id , VarList      
void Parser::varlist(const string& varType)
{
    output << "        Id: " << (currentToken.getType() == TokenType::ID ? currentToken.getValue() : "<ожидается идентификатор>") << endl;

    lastProcessedToken = currentToken;  // Сохраняем текущий токен для возможного вычисления позиции ошибки

    if (currentToken.getType() == TokenType::ID)    // Обрабатываем первый идентификатор в списке
    {
        addDeclaredVariableWithType(currentToken.getValue(), varType);  // Добавляем переменную с типом
        addToPostfix(currentToken.getValue());  // Добавляем переменную для постфиксной записи
        advance(); // Пропускаем идентификатор
    }
    else
    {
        if (currentToken.getType() == TokenType::COMMA)
        {
            output << "        , <неожиданная запятая>" << endl;
            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": неожиданная запятая перед идентификатором";
            errors.push_back(errorMsg);
            advance(); // пропускаем запятую

            // Пытаемся обработать следующий идентификатор
            if (currentToken.getType() == TokenType::ID)
            {
                output << "        Id: " << currentToken.getValue() << endl;
                addDeclaredVariableWithType(currentToken.getValue(), varType);
                advance();
            }
            else
            {
                output << "        Id: <ожидается идентификатор>" << endl;
                if (!match(TokenType::ID, "ожидался идентификатор после ,"))
                {
                    // Восстанавливаемся - пропускаем до точки с запятой
                    skipToSemicolon();
                    return;
                }
            }
        }
        else
        {
            // Другие случаи отсутствия идентификатора
            if (!match(TokenType::ID, "ожидался идентификатор в объявлении"))
            {
                skipToSemicolon();
                return;
            }
        }
    }

    int initialLine = lastProcessedToken.getLine();

    // Обработка дополнительных переменных через запятую
    while (currentToken.getType() == TokenType::COMMA ||
        (currentToken.getType() == TokenType::ID && currentToken.getLine() == initialLine))
    {
        if (currentToken.getType() == TokenType::COMMA)
        {
            output << "        ," << endl;
            match(TokenType::COMMA, "ожидалась ,");
            lastProcessedToken = currentToken;  // Сохраняем позицию после запятой

            output << "        Id: " << (currentToken.getType() == TokenType::ID ? currentToken.getValue() : "<ожидается идентификатор>") << endl;

            if (currentToken.getType() == TokenType::ID)
            {
                addDeclaredVariableWithType(currentToken.getValue(), varType);  // Добавляем все последующие переменные
                addToPostfix(currentToken.getValue());
                advance();
            }
            else // Если нет идентификатора
            {
                if (!match(TokenType::ID, "ожидался идентификатор после ,"))
                    break;
            }
        }
        else if (currentToken.getType() == TokenType::ID)   // Если идентификатор без запятой
        {
            output << "        , <отсутствует>" << endl;
            output << "        Id: " << currentToken.getValue() << endl;

            Token errorToken = currentToken;
            string errorMsg = "строка " + to_string(errorToken.getLine()) + ", позиция " +
                to_string(errorToken.getPosition()) + ": отсутствует ',' между переменными";
            errors.push_back(errorMsg);

            lastProcessedToken = currentToken;
            addDeclaredVariableWithType(currentToken.getValue(), varType);
            addToPostfix(currentToken.getValue());
            advance();  // Пропускаем идентификатор
        }
    }

    if (currentToken.getLine() == initialLine) // Неправильный разделитель
    {
        while (currentToken.getLine() == initialLine &&
            currentToken.getType() != TokenType::COMMA &&
            currentToken.getType() != TokenType::SEMICOLON &&
            currentToken.getType() != TokenType::END_OF_FILE &&
            currentToken.getType() != TokenType::INT &&
            currentToken.getType() != TokenType::DOUBLE &&
            currentToken.getType() != TokenType::RETURN &&
            currentToken.getType() != TokenType::RBRACE &&
            currentToken.getType() != TokenType::ID)
        {
            output << "        <неверный разделитель '" << currentToken.getValue() << "'>" << endl;

            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": ожидалась ',' вместо '" + currentToken.getValue() + "'";
            errors.push_back(errorMsg);

            advance();  // Пропускаем некорректный разделитель

            // Если после разделителя идет идентификатор, обрабатываем его
            if (currentToken.getType() == TokenType::ID)
            {
                output << "        Id: " << currentToken.getValue() << endl;
                addDeclaredVariableWithType(currentToken.getValue(), varType);
                advance();
                continue;   // Продолжаем обработку возможных следующих переменных
            }
        }
    }
}

// Operators → Op | Op Operators
void Parser::operators()
{
    while (currentToken.getType() == TokenType::ID &&   // Обрабатываем все операторы присваивания
        currentToken.getType() != TokenType::END_OF_FILE &&
        currentToken.getType() != TokenType::RETURN &&
        currentToken.getType() != TokenType::RBRACE)
    {
        // Сохраняем текущий идентификатор и заглядываем на следующий токен для анализа контекста
        Token idToken = currentToken;
        Token nextToken = lexer.peekNextToken();

        output << "    Op" << endl;
        op();   // Разбираем оператор присваивания
    }

    // Обработка ошибочных объявлений переменных после операторов
    while (currentToken.getType() == TokenType::INT || currentToken.getType() == TokenType::DOUBLE)
    {
        output << "    Descr <ошибка: объявления после операторов>" << endl;
        string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
            to_string(currentToken.getPosition()) + ": объявление переменных после операторов";
        errors.push_back(errorMsg);

        showErroneousDescription(); // Показываем ошибочное объявление в дереве разбора с пометкой об ошибке

        operators();    // Возвращаемся к разбору операторов после обработки ошибочного объявления
        return; // Выходим после рекурсивного вызова
    }

    // Обработка случая когда есть =, но нет левой части
    if (currentToken.getType() == TokenType::ASSIGN)
    {
        output << "    Op" << endl;
        output << "      Id: <отсутствует>" << endl;

        Token errorToken = currentToken;
        string errorMsg = "строка " + to_string(errorToken.getLine()) + ", позиция " +
            to_string(errorToken.getPosition()) + ": ожидался идентификатор в левой части присваивания";
        errors.push_back(errorMsg);

        advance();

        output << "      =" << endl;
        output << "      Expr" << endl;
        expr(3);

        output << "      ;" << endl;
        if (currentToken.getType() == TokenType::SEMICOLON)
            advance();
        else
            skipToSemicolon();
    }
}

void Parser::op()
{
    string varName = currentToken.getValue();
    output << "      Id: " << varName << endl;

    if (!isVariableDeclared(varName))   // Объявлена ли переменная в левой части присваивания
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": использование необъявленной переменной '" + varName + "'";
        errors.push_back(errorMsg);
    }

    if (!match(TokenType::ID, "ожидался идентификатор"))
        return;

    currentExpression.clear();

    if (currentToken.getType() != TokenType::ASSIGN)    // Проверяем наличие оператора присваивания
    {
        output << "      = <отсутствует>" << endl;
        string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
            to_string(currentToken.getPosition()) + ": ожидался = после идентификатора";
        errors.push_back(errorMsg);

        // Продолжаем разбор выражения даже без =
        if (currentToken.getType() == TokenType::ID ||
            currentToken.getType() == TokenType::INT_NUM ||
            currentToken.getType() == TokenType::DOUBLE_NUM ||
            currentToken.getType() == TokenType::LPAREN ||
            currentToken.getType() == TokenType::ITOD ||
            currentToken.getType() == TokenType::DTOI)
        {
            output << "      Expr" << endl;
            expr(4);

            // Семантическая проверка типов
            string exprType = getExpressionType();
            checkAssignmentType(varName, exprType); // Проверяем типы в присваивании

            addToPostfix(varName);  // Добавляем переменную (левую часть)
            addToPostfix("=");      // Добавляем операцию присваивания
            completeExpression();   // Очищаем для следующего выражения

            lastProcessedToken = currentToken;  // Сохраняем последний токен выражения для вычисления позиции ошибки

            output << "      ;";
            if (currentToken.getType() == TokenType::SEMICOLON)
            {
                output << endl;
                advance();
            }
            else
            {
                output << " <ожидалась ;>" << endl;
                int errorLine = lastProcessedToken.getLine();
                int errorPosition = lastProcessedToken.getPosition() + lastProcessedToken.getValue().length();
                string errorMsg = "строка " + to_string(errorLine) +
                    ", позиция " + to_string(errorPosition) + ": ожидалась ;";
                errors.push_back(errorMsg);
            }
        }
        else    // Невозможно разобрать выражение - пропускаем до точки с запятой
        {
            output << "      ; <ожидалась ;>" << endl;
            skipToSemicolon();
        }
    }
    else
    {
        output << "      =" << endl;
        if (!match(TokenType::ASSIGN, "ожидался ="))
            return;

        output << "      Expr" << endl;

        currentExpression.clear();
        Token lastTokenBeforeExpr = currentToken;   // Сохраняем последний токен перед разбором выражения

        expr(4);    // Разбор выражения

        while (currentToken.getType() == TokenType::RPAREN)     // Обработка всех лишних ')'
        {
            output << "        ) <лишняя>" << endl;
            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": лишняя закрывающаяся скобка";
            errors.push_back(errorMsg);
            advance();
        }

        // Семантическая проверка типов
        string exprType = getExpressionType();
        checkAssignmentType(varName, exprType);

        addToPostfix(varName);
        addToPostfix("=");
        completeExpression(); // очищаем для следующего выражения

        // Проверяем переход на новую строку после выражения
        if (currentToken.getLine() != lastTokenBeforeExpr.getLine())
        {
            output << "      ; <отсутствует>" << endl;
            int errorLine = lastValidToken.getLine();
            int errorPosition = lastValidToken.getPosition() + lastValidToken.getValue().length(); // Позиция последнего токена + длина

            string errorMsg = "строка " + to_string(errorLine) +
                ", позиция " + to_string(errorPosition) + ": ожидалась ;";
            errors.push_back(errorMsg);
        }
        else if (currentToken.getType() != TokenType::SEMICOLON)
        {
            // Остались на той же строке, но нет точки с запятой
            output << "      ; <отсутствует>" << endl;
            int errorPosition = currentToken.getPosition() + currentToken.getValue().length();
            string errorMsg = "строка " + to_string(currentToken.getLine()) +
                ", позиция " + to_string(errorPosition) + ": ожидалась ;";
            errors.push_back(errorMsg);
            skipToSemicolon();
        }
        else
        {
            output << "      ;" << endl;
            advance();
        }
    }
}

// Expr → SimpleExpr | SimpleExpr + Expr | SimpleExpr - Expr
void Parser::expr(int indentLevel)
{
    string indent = string(indentLevel * 2, ' ');

    output << indent << "SimpleExpr" << endl;   // Разбираем простое выражение
    simpleExpr(indentLevel + 1);

    string leftType = getExpressionType();  // Получаем тип левого операнда
    if (currentToken.getType() == TokenType::PLUS || currentToken.getType() == TokenType::MINUS)
    {
        string op = currentToken.getValue();
        // Поддерживаемые операции: сложение и вычитание
        output << indent << currentToken.getValue() << endl;
        advance();  // Пропускаем оператор

        output << indent << "Expr" << endl;
        expr(indentLevel + 1);  // Рекурсивно разбираем выражение

        string rightType = getExpressionType(); // Получаем тип правого операнда
        checkBinaryOperationTypes(leftType, rightType, op); // Проверяем совместимость типов в операции
        addToPostfix(op);                       // Добавляем операцию после операндов
        currentExpression.push_back(op);        // Собираем текущее выражение
    }
    else if (currentToken.getType() == TokenType::MULT || currentToken.getType() == TokenType::DIV)
    {
        string op = currentToken.getValue();
        // Неподдерживаемые операции: умножение и деление
        output << indent << currentToken.getValue() << " <неподдерживаемая операция>" << endl;
        string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
            to_string(currentToken.getPosition()) + ": операция '" + currentToken.getValue() + "' не поддерживается";
        errors.push_back(errorMsg);
        advance();

        output << indent << "Expr" << endl;
        expr(indentLevel + 1);
        addToPostfix(op);
        currentExpression.push_back(op);
    }
}

// SimpleExpr → Id | Const | ( Expr ) | itod ( Expr ) | dtoi ( Expr )
void Parser::simpleExpr(int indentLevel)
{
    string indent = string(indentLevel * 2, ' ');

    switch (currentToken.getType())
    {
    case TokenType::ID:
    {
        string identifierName = currentToken.getValue();
        output << indent << "Id: " << identifierName;

        // Проверяем, является ли это вызовом функции (следующий токен - '(')
        Token nextToken = lexer.peekNextToken();
        if (nextToken.getType() == TokenType::LPAREN)
        {
            // Это вызов функции
            output << " <вызов функции>" << endl;
            string currentFunctionCall = identifierName;    // Сохраняем имя функции для последующей проверки
            if (identifierName == "itod")
                currentExpression.push_back("itod");
            else if (identifierName == "dtoi")
                currentExpression.push_back("dtoi");
            else
            {
                string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                    to_string(currentToken.getPosition()) + ": вызов неизвестной функции '" +
                    identifierName + "'";
                errors.push_back(errorMsg);
            }

            advance(); // пропускаем имя функции

            output << indent << "(" << endl;
            match(TokenType::LPAREN, "ожидалась ( после " + identifierName);

            output << indent << "Expr" << endl;
            expr(indentLevel + 1);

            string argType = getExpressionType();
            checkFunctionArgumentType(currentFunctionCall, argType);    // Проверяем соответствие типа аргумента
            currentExpression.push_back(currentFunctionCall);           // Добавляем вызов функции в текущее выражение

            output << indent << ")" << endl;
            if (!match(TokenType::RPAREN, "ожидалась ) после выражения в " + identifierName))
            {
                // Обработка лишних скобок
                while (currentToken.getType() == TokenType::RPAREN)
                {
                    output << indent << ") <лишняя>" << endl;
                    string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                        to_string(currentToken.getPosition()) + ": лишняя закрывающаяся скобка";
                    errors.push_back(errorMsg);
                    advance();
                }
            }
            addToPostfix(identifierName);
        }
        else
        {
            // Это обычная переменная
            output << (isVariableDeclared(identifierName) ? "" : " <необъявленная переменная>") << endl;
            if (!isVariableDeclared(identifierName))    // Проверка объявления переменной
            {
                string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                    to_string(currentToken.getPosition()) + ": использование необъявленной переменной '" +
                    identifierName + "' в выражении";
                errors.push_back(errorMsg);
            }
            addToPostfix(identifierName);                   // Добавляем имя переменной в постфиксную запись 
            currentExpression.push_back(identifierName);    // Добавляем имя переменной в currentExpression
            match(TokenType::ID, "ожидался идентификатор");
        }
        break;
    }

    case TokenType::INT_NUM:
        output << indent << "Const: " << currentToken.getValue() << " (int)" << endl;
        addToPostfix(currentToken.getValue());
        currentExpression.push_back(currentToken.getValue());
        advance();
        break;

    case TokenType::DOUBLE_NUM:
        output << indent << "Const: " << currentToken.getValue() << " (double)" << endl;
        addToPostfix(currentToken.getValue());
        currentExpression.push_back(currentToken.getValue());
        advance();
        break;

    case TokenType::LPAREN:
        output << indent << "(" << endl;
        match(TokenType::LPAREN, "ожидалась (");

        output << indent << "Expr" << endl;
        expr(indentLevel + 1);

        output << indent << ")" << endl;
        if (!match(TokenType::RPAREN, "ожидалась )"))
        {
            // Проверяем лишние закрывающие скобки
            while (currentToken.getType() == TokenType::RPAREN)
            {
                output << indent << ") <лишняя>" << endl;
                string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                    to_string(currentToken.getPosition()) + ": лишняя закрывающаяся скобка";
                errors.push_back(errorMsg);
                advance();
            }
        }
        break;

    case TokenType::ITOD:
    case TokenType::DTOI:
    {
        // Определяем имя функции преобразования типа
        string funcName = (currentToken.getType() == TokenType::ITOD) ? "itod" : "dtoi";
        output << indent << funcName << endl;

        advance();

        output << indent << "(" << endl;
        if (!match(TokenType::LPAREN, "ожидалась ( после " + funcName))
            return;

        output << indent << "Expr" << endl; // Разбираем выражение внутри скобок
        expr(indentLevel + 1);

        string argType = getExpressionType();
        checkFunctionArgumentType(funcName, argType);   // Проверяем соответствие типа аргумента
        currentExpression.push_back(funcName);  // Добавляем вызов функции в текущее выражение

        output << indent << ")" << endl;
        if (!match(TokenType::RPAREN, "ожидалась ) после выражения в " + funcName))
        {
            while (currentToken.getType() == TokenType::RPAREN) // Обрабатываем лишние закрывающие скобки
            {
                output << indent << ") <лишняя>" << endl;
                string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                    to_string(currentToken.getPosition()) + ": лишняя закрывающаяся скобка";
                errors.push_back(errorMsg);
                advance();
            }
        }
        addToPostfix(funcName);
        break;
    }

    default:
        output << indent << "<неожиданный токен '" << currentToken.getValue() << "'>" << endl;
        error("ожидалось простое выражение");
        break;
    }
}

void Parser::skipToSemicolon()  // Пропуск токенов до точки с запятой или других значимых разделителей
{
    while (currentToken.getType() != TokenType::SEMICOLON &&
        currentToken.getType() != TokenType::RETURN &&
        currentToken.getType() != TokenType::RBRACE &&
        currentToken.getType() != TokenType::END_OF_FILE)
    {
        if (currentToken.getType() == TokenType::RPAREN)    // Пропускаем лишние закрывающиеся скобки
        {
            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": лишняя закрывающаяся скобка";
            errors.push_back(errorMsg);
        }
        advance();  // Переходим к следующему токену
    }

    // Если нашли точку с запятой, пропускаем ее
    // Это позволяет продолжить разбор со следующего оператора
    if (currentToken.getType() == TokenType::SEMICOLON)
        advance();
}

void Parser::showErroneousDescription() // Метод для отображения ошибочного объявления в дереве
{
    output << "      Type: ";

    if (currentToken.getType() == TokenType::INT || currentToken.getType() == TokenType::DOUBLE)
    {
        // Определяем имя типа и выводим с сообщением об ошибке
        string typeName = (currentToken.getType() == TokenType::INT) ? "int" : "double";
        output << typeName << " <ошибка: после операторов>" << endl;
        advance();
    }
    else
        output << "<ожидается тип>" << endl;

    output << "      VarList" << endl;

    // Полностью обрабатываем список переменных ошибочного объявления
    bool firstVariable = true;
    while (currentToken.getType() != TokenType::SEMICOLON &&
        currentToken.getType() != TokenType::END_OF_FILE &&
        currentToken.getType() != TokenType::RETURN &&
        currentToken.getType() != TokenType::RBRACE)
    {
        if (currentToken.getType() == TokenType::ID)    // Обработка идентификатора переменной
        {
            if (!firstVariable)
                output << "        ," << endl;  // Выводим запятую перед каждой последующей переменной
            output << "        Id: " << currentToken.getValue() << " <ошибка: после операторов>" << endl;
            addDeclaredVariable(currentToken.getValue());
            advance();              // Пропускаем идентификатор
            firstVariable = false;  // Следующая переменная не будет первой
        }
        else if (currentToken.getType() == TokenType::COMMA)    // Обработка запятой
        {
            output << "        ," << endl;
            advance();  // Пропускаем запятую

            if (currentToken.getType() == TokenType::ID)    // Если после запятой идет идентификатор
            {
                output << "        Id: " << currentToken.getValue() << " <ошибка: после операторов>" << endl;
                addDeclaredVariable(currentToken.getValue());
                advance();
            }
        }
        else
            advance();  // Пропускаем непонятные токены
    }

    output << "      ;" << endl;
    if (currentToken.getType() == TokenType::SEMICOLON) // Пропускаем точку с запятой, если есть
        advance();
}

void Parser::processVariableListForUnknownType()    // Обработка переменных с неизвестным типом
{
    if (currentToken.getType() == TokenType::ID)    // Обрабатываем первую переменную
    {
        output << "        Id: " << currentToken.getValue() << endl;
        addDeclaredVariable(currentToken.getValue());
        addToPostfix(currentToken.getValue());
        advance();  // Пропускаем идентификатор
    }
    else
    {
        if (!match(TokenType::ID, "ожидался идентификатор в объявлении"))
            return;
    }

    int initialLine = currentToken.getLine();

    // Обрабатываем остальные переменные через запятую
    while (currentToken.getType() == TokenType::COMMA ||
        (currentToken.getType() == TokenType::ID && currentToken.getLine() == initialLine))
    {
        if (currentToken.getType() == TokenType::COMMA)
        {
            output << "        ," << endl;
            match(TokenType::COMMA, "ожидалась ,");

            if (currentToken.getType() == TokenType::ID)
            {
                output << "        Id: " << currentToken.getValue() << endl;
                addDeclaredVariable(currentToken.getValue());
                addToPostfix(currentToken.getValue());
                advance();
            }
            else
            {
                if (!match(TokenType::ID, "ожидался идентификатор после ,"))
                    break;
            }
        }
        else if (currentToken.getType() == TokenType::ID)   // Если идентификатор без запятой
        {
            output << "        , <отсутствует>" << endl;
            output << "        Id: " << currentToken.getValue() << endl;

            Token errorToken = currentToken;
            string errorMsg = "строка " + to_string(errorToken.getLine()) + ", позиция " +
                to_string(errorToken.getPosition()) + ": отсутствует ',' между переменными";
            errors.push_back(errorMsg);

            addDeclaredVariable(currentToken.getValue());
            addToPostfix(currentToken.getValue());
            advance();  // Пропускаем идентификатор
        }
    }

    // Обработка неправильных разделителей
    if (currentToken.getLine() == initialLine)
    {
        while (currentToken.getLine() == initialLine &&
            currentToken.getType() != TokenType::COMMA &&
            currentToken.getType() != TokenType::SEMICOLON &&
            currentToken.getType() != TokenType::END_OF_FILE &&
            currentToken.getType() != TokenType::INT &&
            currentToken.getType() != TokenType::DOUBLE &&
            currentToken.getType() != TokenType::RETURN &&
            currentToken.getType() != TokenType::RBRACE &&
            currentToken.getType() != TokenType::ID)
        {
            output << "        <неверный разделитель '" << currentToken.getValue() << "'>" << endl;
            string errorMsg = "строка " + to_string(currentToken.getLine()) + ", позиция " +
                to_string(currentToken.getPosition()) + ": ожидалась ',' вместо '" + currentToken.getValue() + "'";
            errors.push_back(errorMsg);
            advance();
        }
    }
}

void Parser::addDeclaredVariable(const string& varName) // Используется для добавления переменных при ошибочных объявлениях
{
    // Создаем токен для переменной
    Token varToken(TokenType::ID, varName, currentToken.getLine(), currentToken.getPosition());

    if (declaredVariables->contains(varName))   // Проверяем, не объявлена ли переменная ранее
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": повторное объявление переменной '" + varName + "'";
        errors.push_back(errorMsg);
    }
    else
        declaredVariables->insert(varToken);    // Если переменная не объявлена - добавляем в таблицу
}

void Parser::addDeclaredVariableWithType(const string& varName, const string& type) // Используется для добавления переменных, у которых известен тип (int, double)
{
    // Создаем токен для переменной
    Token varToken(TokenType::ID, varName, currentToken.getLine(), currentToken.getPosition());

    if (declaredVariables->contains(varName))   // Проверяем, не объявлена ли переменная ранее
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": повторное объявление переменной '" + varName + "'";
        errors.push_back(errorMsg);
    }
    else
        declaredVariables->insertWithType(varToken, type);
}

// Операнды(переменные, константы) помещаются в стек
// Операции и функции извлекают операнды из стека, вычисляют тип результата и помещают его обратно
// В конце в стеке остается тип всего выражения
string Parser::getExpressionType()  // Определение типа выражения
{
    if (currentExpression.empty())  // Если текущее выражение пустое, возвращаем неизвестный тип
        return "unknown";

    stack<string> typeStack;    // Стек для хранения типов операндов

    for (const auto& token : currentExpression) // Проходим по всем токенам в текущем выражении
    {
        string tokenType = getVariableType(token);
        if (!tokenType.empty())                 // Если токен является объявленной переменной
            typeStack.push(tokenType);
        else if (isdigit(token[0]))                             // Если токен начинается с цифры - это числовая константа
        {
            if (token.find('.') != string::npos) // Проверяем, содержит ли константа точку
                typeStack.push("double");
            else
                typeStack.push("int");
        }
        else if (token == "itod")   // Функция itod: берет int из стека, возвращает double
        {
            if (!typeStack.empty()) // Для функции должен быть хотя бы один аргумент в стеке
            {
                string argType = typeStack.top();   // Извлекаем тип аргумента из вершины стека
                typeStack.pop();                    // Удаляем аргумент из стека
                typeStack.push("double");           // Функция itod возвращает double
            }
        }
        else if (token == "dtoi")   // Функция dtoi: берет double из стека, возвращает int
        {
            if (!typeStack.empty()) // Для функции должен быть хотя бы один аргумент в стеке
            {
                string argType = typeStack.top();   // Извлекаем тип аргумента из вершины стека
                typeStack.pop();                    // Удаляем аргумент из стека
                typeStack.push("int");              // Функция dtoi возвращает int
            }
        }
        else if (token == "+" || token == "-") // Если токен - операция сложения или вычитания
        {
            if (typeStack.size() >= 2)          // Нужно как минимум два операнда в стеке
            {
                string type2 = typeStack.top(); // Правый операнд
                typeStack.pop();
                string type1 = typeStack.top(); // Левый операнд
                typeStack.pop();

                if (type1 == "double" || type2 == "double") // Если хотя бы один операнд имеет тип double, результат - double
                    typeStack.push("double");
                else
                    typeStack.push("int");
            }
        }
    }

    // Если после обработки всех токенов стек пуст, возвращаем "unknown"
    // Иначе возвращаем тип результата, оставшийся в вершине стека
    return typeStack.empty() ? "unknown" : typeStack.top();
}

void Parser::checkAssignmentType(const string& varName, const string& exprType) // Проверка соответствия типов в операции присваивания
{
    string varType = getVariableType(varName);
    if (varType.empty())
        return; // Переменная не найдена

    // Сравниваем тип переменной (левая часть присваивания) с типом выражения (правая часть присваивания)
    if (varType != exprType)
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": несоответствие типов в присваивании '" + varName +
            "' (" + varType + ") = выражение (" + exprType + ")";
        errors.push_back(errorMsg);
    }
}

void Parser::checkFunctionReturnType(const string& returnVar)
{
    string returnType = getVariableType(returnVar);
    if (returnType.empty())
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": переменная возврата '" + returnVar + "' не объявлена";
        errors.push_back(errorMsg);
        return;
    }

    if (returnType != currentFunctionType)          // Сравниваем тип переменной возврата с типом функции
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": несоответствие типа возврата '" + returnType +
            "' с типом функции '" + currentFunctionType + "'";
        errors.push_back(errorMsg);
    }
}

void Parser::processFunctionCall(const string& funcName)    // Проверка вызова функции на корректность имени
{
    if (funcName != "itod" && funcName != "dtoi")   // Проверяем, является ли имя функции одним из разрешенных
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": вызов неизвестной функции '" + funcName + "'";
        errors.push_back(errorMsg);
    }
}

void Parser::addToPostfix(const string& token)  // Добавление токена в постфиксную запись
{
    postfixCode.push_back(token);
}

void Parser::generatePostfix()
{
    output << endl << "=== ПОСТФИКСНАЯ ЗАПИСЬ ===" << endl;

    if (postfixCode.empty())
    {
        output << "<нет операций>" << endl;
        return;
    }

    /*if (!currentFunctionType.empty() && !currentFunctionName.empty())
        output << currentFunctionType << " " << currentFunctionName << " FUNCTION" << endl;
    else
        output << "unknown unknown FUNCTION" << endl;*/

    vector<string> currentLine; // Вектор для накопления токенов текущей строки вывода
    bool inDeclaration = false; // Флаг, указывающий, что мы находимся внутри объявления переменных
    int varCount = 0;           // Счетчик переменных в текущем объявлении

    for (size_t i = 0; i < postfixCode.size(); ++i)
    {
        const string& token = postfixCode[i];   // Получаем текущий токен по индексу

        if (token == "DECLARE") // Проверяем, является ли токен началом объявления
        {
            if (!currentLine.empty())   // Если уже накопили какую-то строку, выводим ее
            {
                if (inDeclaration && varCount > 0)
                {
                    currentLine.push_back(to_string(varCount + 1));
                    currentLine.push_back("DECL");
                }

                for (size_t j = 0; j < currentLine.size(); ++j) // Вывод собранной строки
                {
                    output << currentLine[j];
                    if (j < currentLine.size() - 1)
                        output << " ";
                }
                output << endl;
                currentLine.clear();
            }

            inDeclaration = true;   // Начинаем новое объявление
            varCount = 0;
        }
        else if (inDeclaration) // Если находимся внутри объявления
        {
            if (varCount == 0 && (token == "int" || token == "double"))
                currentLine.push_back(token); // Добавляем тип
            else
            {
                currentLine.push_back(token); // Добавляем имя переменной
                varCount++; // Увеличиваем счетчик переменных
            }

            if (i + 1 < postfixCode.size()) // Проверяем, не закончилось ли объявление
            {
                string nextToken = postfixCode[i + 1];
                // Объявление считается законченным, если следующий токен:
                // 1. Новое DECLARE (начало следующего объявления)
                // 2. RETURN (конец функции)
                // 3. "=" (начало операции присваивания)
                // 4. Число (начало выражения)
                if (nextToken == "DECLARE" || nextToken == "RETURN" || nextToken == "=" || isdigit(nextToken[0]))
                {
                    currentLine.push_back(to_string(varCount + 1));
                    currentLine.push_back("DECL");

                    for (size_t j = 0; j < currentLine.size(); ++j) // Вывод завершенного объявления
                    {
                        output << currentLine[j];
                        if (j < currentLine.size() - 1)
                            output << " ";
                    }
                    output << endl;
                    currentLine.clear();
                    inDeclaration = false;
                    varCount = 0;
                }
            }
        }
        else if (token == "=" || token == "RETURN")
        {
            currentLine.push_back(token);   // Добавляем завершающий токен операции к строке
            for (size_t j = 0; j < currentLine.size(); ++j) // Вывод завершенной операции
            {
                output << currentLine[j];
                if (j < currentLine.size() - 1)
                    output << " ";
            }
            output << endl;
            currentLine.clear();
        }
        else    // Обычный токен внутри операции
            currentLine.push_back(token);
    }

    if (!currentLine.empty())   // Вывод последней строки, если она осталась невыведенной
    {
        if (inDeclaration && varCount > 0)  // Если это объявление
        {
            currentLine.push_back(to_string(varCount + 1));
            currentLine.push_back("DECL");
        }

        for (size_t j = 0; j < currentLine.size(); ++j)
        {
            output << currentLine[j];
            if (j < currentLine.size() - 1)
                output << " ";
        }
        output << endl;
    }
}

void Parser::completeExpression()   // Завершение обработки текущего выражения
{
    currentExpression.clear();
}

bool Parser::isVariableDeclared(const string& varName)  // Проверка, была ли переменная объявлена ранее
{
    return declaredVariables->contains(varName);
}

void Parser::clearDeclaredVariables()   // Очистка данных
{
    declaredVariables->clear();
}

// Проверка соответствия типа аргумента, передаваемого в функцию преобразования
void Parser::checkFunctionArgumentType(const string& funcName, const string& argType)
{
    if (funcName == "itod")     // Проверяем, является ли функция функцией itod
    {
        if (argType != "int")   // Функция itod ожидает аргумент типа int
        {
            string errorMsg = "строка " + to_string(currentToken.getLine()) +
                ": функция 'itod' ожидает аргумент типа 'int', получен '" + argType + "'";
            errors.push_back(errorMsg);
        }
    }
    else if (funcName == "dtoi")
    {
        if (argType != "double")
        {
            string errorMsg = "строка " + to_string(currentToken.getLine()) +
                ": функция 'dtoi' ожидает аргумент типа 'double', получен '" + argType + "'";
            errors.push_back(errorMsg);
        }
    }
}

// Проверка совместимости типов операндов в бинарной операции
void Parser::checkBinaryOperationTypes(const string& leftType, const string& rightType, const string& op)
{
    // Если типы разные - это неявное преобразование
    if (leftType != rightType && leftType != "unknown" && rightType != "unknown")
    {
        string errorMsg = "строка " + to_string(currentToken.getLine()) +
            ": неявное преобразование типов в операции '" + op +
            "' между " + leftType + " и " + rightType;
        errors.push_back(errorMsg);
    }
}