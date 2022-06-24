#include "Evaluator.h"
#include "EvaluatorFunctions.h"

Evaluator::Evaluator(const QVariantMap* symbols, const QVariantMap* variables, const QueryFieldValue& queryFieldValue)
{
    reset();

    m_symbols = symbols;
    m_variables = variables;
    m_queryFieldValue = queryFieldValue;
}

Evaluator::~Evaluator()
{
    reset();

    m_symbols = nullptr;
    m_variables = nullptr;
    m_queryFieldValue = nullptr;
}

void Evaluator::reset()
{
    m_parseError = m_abort = false;

    m_program.clear();
    m_contextRecordUid.clear();
    m_contextFieldUid.clear();

    m_pword = m_pwordstart = nullptr;
    m_tokenType = TokenType::None;
    m_value.clear();
    m_word.clear();
}

QVariant Evaluator::evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid)
{
    reset();

    m_contextRecordUid = contextRecordUid;
    m_contextFieldUid = contextFieldUid;

    m_program = expression.toLatin1();
    m_pword = m_pwordstart = m_program.data();

    auto result = commaList(true);

    if (m_tokenType != TokenType::End)
    {
        parseFailure("Unexpected end");
    }

    return m_parseError ? QVariant() : result;
}

Evaluator::TokenType Evaluator::getToken(const bool ignoreSign)
{
    if (m_parseError)
    {
        return m_tokenType = TokenType::End;
    }

    // Clear the current word.
    m_word.clear();

    // Skip spaces.
    while (*m_pword && isspace(*m_pword))
    {
        ++m_pword;
    }

    // Remember current starting position.
    m_pwordstart = m_pword;

    // Check for unterminated statements.
    if ((*m_pword == 0) && (m_tokenType == TokenType::End))
    {
        parseFailure("Unterminated statement");
        return m_tokenType = TokenType::End;
    }

    // First character in new m_word.
    char firstchar = *m_pword;
    if (firstchar == 0)    // Stop at end of file.
    {
        m_word = "<end>";
        return m_tokenType = TokenType::End;
    }

    // Second character in new m_word.
    char nextchar = *(m_pword + 1);

    // Look for number.
    if ((!ignoreSign && (firstchar == '+' || firstchar == '-') && isdigit(nextchar)) || isdigit (firstchar))
    {
        // Skip sign.
        if ((firstchar == '+' || firstchar == '-'))
        {
            m_pword++;
        }

        while (isdigit (*m_pword) || *m_pword == '.')
        {
            m_pword++;
        }

        // Allow for 1.01e+12.
        if (*m_pword == 'e' || *m_pword == 'E')
        {
            // Skip 'e'
            m_pword++;
            if ((*m_pword  == '+' || *m_pword  == '-'))
            {
                // Skip sign after e.
                m_pword++;
            }

            // Now digits after e.
            while (isdigit (*m_pword))
            {
                m_pword++;
            }
        }

        // Get double value.
        m_word = QString(m_pwordstart).left(m_pword - m_pwordstart);
        m_value = m_word.toDouble();

        return (m_tokenType = TokenType::Number);
    }

    // Special test for 2-character sequences: <=, >=, ==, !=, +=, -=, /=, *=.
    if (nextchar == '=')
    {
        switch (firstchar)
        {
        // Comparisons.
        case '=': m_tokenType = TokenType::Equal; break;
        case '<': m_tokenType = TokenType::LessThanOrEqual; break;
        case '>': m_tokenType = TokenType::GreaterThanOrEqual; break;
        case '!': m_tokenType = TokenType::NotEqual; break;

        // Assignments.
        case '+': m_tokenType = TokenType::AssignAdd; break;
        case '-': m_tokenType = TokenType::AssignSubtract; break;
        case '*': m_tokenType = TokenType::AssignMultiply; break;
        case '/': m_tokenType = TokenType::AssignDivide; break;

        // None of the above.
        default:  
            m_tokenType = TokenType::None;
            break;
        } 

        if (m_tokenType != TokenType::None)
        {
            // Skip both characters.
            m_word = QString(m_pwordstart).left(2);
            m_pword += 2;
            return m_tokenType;
        } 
    } 

    auto next = QString(m_pwordstart);

    // Look for 'or'.
    if (next.startsWith("or") && next.length() > 2 && (next[2] == ' ' || next[2] == '('))
    {
        m_word = QString(m_pwordstart).left(2);
        m_pword += 2;   // skip both characters
        return m_tokenType = TokenType::Or;
    }

    // Look for 'and'.
    if (next.startsWith("and") && next.length() > 3 && (next[3] == ' ' || next[3] == '('))
    {
        m_word = QString(m_pwordstart).left(3);
        m_pword += 3;   // skip three characters
        return m_tokenType = TokenType::And;
    }

    // Look for 'div'.
    if (next.startsWith("div") && next.length() > 3 && (next[3] == ' ' || next[3] == '('))
    {
        m_word = QString(m_pwordstart).left(3);
        m_pword += 3;   // skip three characters
        return m_tokenType = TokenType::Divide;
    }

    // Look for a field.
    if (next.startsWith("${"))
    {
        m_pword += 2;
        while (*m_pword && (*m_pword != '}'))
        {
            ++m_pword;
        }

        m_word = QString(m_pwordstart + 2).left(m_pword - m_pwordstart - 2);
        m_pword += 1;

        return m_tokenType = TokenType::FieldValue;
    }

    // Look for a current field.
    if (firstchar == '.')
    {
        m_word = m_contextFieldUid;
        m_pword += 1;
        return m_tokenType = TokenType::FieldValue;
    }

    // Single-character symbols.
    switch (firstchar)
    {
    case '=':
    case '<':
    case '>':
    case '+':
    case '-':
    case '/':
    case '*':
    case '(':
    case ')':
    case '[':
    case ']':
    case ',':
    case '!':
        m_word = QString(m_pwordstart).at(0);
        ++m_pword;   // skip it
        return m_tokenType = TokenType(firstchar);

    case '\'':
    case '\"':
        m_pword += 1;
        while (*m_pword && (*m_pword != firstchar))
        {
            ++m_pword;
        }

        m_word = QString(m_pwordstart + 1).left(m_pword - m_pwordstart - 1);
        m_pword += 1;
        m_value = m_word;

        return m_tokenType = TokenType::Text;
    } 

    if (!isalpha(firstchar))
    {
        parseFailure("Expected alphanumeric, but got " + QString(firstchar));
        return m_tokenType = TokenType::End;
    }

    // We have a word (starting with A-Z) - pull it out
    while (isalnum(*m_pword) || *m_pword == '_')
    {
        ++m_pword;
    }

    // If we have a '-' followed by another letter, then coalesce.
    for (; !m_parseError; )
    {
        if (*m_pword == '-' && isalpha(*(m_pword + 1)))
        {
            ++m_pword;

            while (isalnum(*m_pword) || *m_pword == '_')
            {
                ++m_pword;
            }
        }
        else
        {
            break;
        }
    }

    m_word = QString(m_pwordstart).left(m_pword - m_pwordstart);

    return m_tokenType = TokenType::Symbol;
}   

QVariant Evaluator::primary(const bool Value)   // primary (base) tokens
{
    if (Value)
    {
        // Lookahead  
        getToken();
    }

    switch (m_tokenType)
    {
    case TokenType::Number:
        {
            double v = m_value.toDouble();
            getToken(true);  // get next one (one-token lookahead)
            return v;
        }

    case TokenType::Text:
    {
        auto v = m_value;
        getToken(true);
        return v;
    }

    case TokenType::FieldValue:
    {
        if (!m_queryFieldValue)
        {
            parseFailure("Field value required but callback not provided");
            return QVariant();
        }

        // Note that if the field value is not found, it will be an invalid variant.
        auto v = m_queryFieldValue(m_contextRecordUid, m_word);
        getToken(true);
        return v;
    }

    case TokenType::ArrayStart:
    {
        auto items = QVariantList();

        for (; !m_parseError; )
        {
            getToken(true);
            if (m_tokenType == TokenType::ArrayClose)
            {
                getToken(true);
                break;
            }

            items.append(expression(false));

            if (m_tokenType == TokenType::Comma)
            {
                continue;
            }
            else if (m_tokenType == TokenType::ArrayClose)
            {
                getToken(true);
                break;
            }

            break;
        }

        return items;
    }

    case TokenType::Symbol:
        {
            QString word(m_word);
            getToken(true);

            // Functions.
            if (m_tokenType == TokenType::LeftBrace)
            {
                auto params = QVariantList();

                for (; !m_parseError; )
                {
                    getToken(true);
                    if (m_tokenType == TokenType::RightBrace)
                    {
                        getToken(true);
                        break;
                    }

                    params.append(expression(false));

                    if (m_tokenType == TokenType::Comma)
                    {
                        continue;
                    }
                    else if (m_tokenType == TokenType::RightBrace)
                    {
                        getToken(true);
                        break;
                    }
                }

                if (!m_parseError)
                {
                    if (s_functionTable.contains(word))
                    {
                        auto result = QVariant();
                        if (s_functionTable[word](params, result))
                        {
                            return result;
                        }
                    }

                    if (m_symbols->contains(word) && params.isEmpty())
                    {
                        return m_symbols->value(word);
                    }

                    parseFailure("Function not found: " + word);
                }

                return 0;
            }

            auto value = QVariant();
            auto found = false;

            // Check for a variable.
            if (m_variables)
            {
                if (m_variables->contains(word))
                {
                    value = m_variables->value(word);
                    found = true;
                }
            }

            // Check for a symbol.
            if (!found && m_symbols->contains(word))
            {
                value = m_symbols->value(word);
                found = true;
            }

            // Must be a symbol in the symbol table.
            if (!found)
            {
                parseFailure("Unknown symbol: " + word);
                return 0;
            }

            auto valueDouble = value.toDouble();

            switch (m_tokenType)
            {
            case TokenType::AssignAdd:      return valueDouble + expression(true).toDouble();
            case TokenType::AssignSubtract: return valueDouble - expression(true).toDouble();
            case TokenType::AssignMultiply: return valueDouble * expression(true).toDouble();
            case TokenType::AssignDivide:
                {
                    double d = expression(true).toDouble();
                    if (d == 0.0)
                    {
                        // Divide by zero
                        return 0;
                    }
                    else
                    {
                        return valueDouble / d;
                    }
                }
            default: break;
            }

            return value;
        }

    case TokenType::Minus:
        return - primary(true).toDouble();

    case TokenType::Not:
        return (primary(true) == 0.0) ? 1.0 : 0.0;

    case TokenType::LeftBrace:
        {
            auto v = commaList(true);
            checkToken(TokenType::RightBrace);
            getToken();
            return v;
        }

    default:   
        parseFailure("Unknown token");
        return 0;
    } 
} 

QVariant Evaluator::term(const bool Value)
{
    auto left = primary(Value);

    for (;;)
    {
        switch (m_tokenType)
        {
        case TokenType::Multiply:
            left = doubleVariant(left) * doubleVariant(primary(true));
            break;

        case TokenType::Divide:
            {
                double d = doubleVariant(primary(true));
                if (d == 0.0)
                {
                    left = 0;
                }
                else
                {
                    left = doubleVariant(left) / d;
                }
            }
            break;

        default:    
            return left;
        }
    }   
} 

QVariant Evaluator::addSubtract(const bool Value)  // add and subtract
{
    auto left = term(Value);

    for (;;)
    {
        switch (m_tokenType)
        {
        case TokenType::Plus:
            left = doubleVariant(left) + doubleVariant(term(true));
            break;

        case TokenType::Minus:
            left = doubleVariant(left) - doubleVariant(term(true));
            break;

        default:    
            return left;
        }
    }  
} 

QVariant Evaluator::comparison(const bool Value)
{
    auto left = addSubtract(Value);

    for (;;)
    {
        switch (m_tokenType)
        {
        case TokenType::Compare:
            left = compareVariants(left, addSubtract(true));
            break;

        case TokenType::LessThan:
            left = doubleVariant(left) < doubleVariant(addSubtract(true));
            break;

        case TokenType::GreaterThan:
            left = doubleVariant(left) > doubleVariant(addSubtract(true));
            break;

        case TokenType::LessThanOrEqual:
            left = doubleVariant(left) <= doubleVariant(addSubtract(true));
            break;

        case TokenType::GreaterThanOrEqual:
            left = doubleVariant(left) >= doubleVariant(addSubtract(true));
            break;

        case TokenType::Equal:
            left = fabs(doubleVariant(left) - doubleVariant(addSubtract(true))) < 0.000001;
            break;

        case TokenType::NotEqual:
            left = !compareVariants(left, addSubtract(true));
            break;

        default:    
            return left;
        } 
    }   
} 

QVariant Evaluator::expression(const bool Value)
{
    auto left = comparison(Value);

    for (;;)
    {
        switch (m_tokenType)
        {
        case TokenType::And:
            {
                // Don't want short-circuit evaluation.
                double d = comparison(true).toDouble();
                left = (left != 0.0) && (d != 0.0); 
            }
            break;

        case TokenType::Or:
            {
                // Don't want short-circuit evaluation.
                double d = comparison(true).toDouble();
                left = (left != 0.0) || (d != 0.0); 
            }
            break;

        default:    
            return left;
        } 
    }
} 

QVariant Evaluator::commaList(const bool Value)
{
    auto left = expression(Value);

    for (;;)
    {
        switch (m_tokenType)
        {
        case TokenType::Comma:
            left = expression(true);
            break; 

        default:    
            return left;
        } 
    }   
} 

void Evaluator::checkToken(const TokenType Wanted)
{
    if (m_tokenType != Wanted)
    {
        parseFailure("Wrong token");
    }
}

void Evaluator::parseFailure(const QString& message)
{
    if (!m_abort)
    {
        qDebug() << message;
    }

    m_parseError = true;
}

bool Evaluator::compareVariants(const QVariant& v1, const QVariant& v2)
{
    if (!v1.isValid() || !v2.isValid())
    {
        // Invalid variants cannot be equal.
        return false;
    }

    auto value1 = v1;
    auto value2 = v2;

    if (value1.userType() == QMetaType::Type::QVariantList)
    {
        value1 = value1.toList().first();
    }

    if (value2.userType() == QMetaType::Type::QVariantList)
    {
        value2 = value2.toList().first();
    }

    auto d1okay = false;
    auto d1 = value1.toDouble(&d1okay);
    auto d2okay = false;
    auto d2 = value2.toDouble(&d2okay);
    if (d1okay && d2okay)
    {
        return fabs(d1 - d2) < 0.000001;
    }
    else
    {
        return value1 == value2;
    }
}

double Evaluator::doubleVariant(const QVariant& v)
{
    if (v.userType() == QMetaType::Type::QVariantList)
    {
        return v.toList().first().toDouble();
    }

    return v.toDouble();
}
