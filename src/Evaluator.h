#pragma once
#include "pch.h"

namespace CyberTracker {

class Evaluator
{
public:
    typedef std::function<QVariant(const QString& contextRecordUid, const QString& findFieldUid)> QueryFieldValue;

    Evaluator(const QVariantMap* symbols = nullptr, const QVariantMap* variables = nullptr, const QueryFieldValue& queryFieldValue = nullptr);
    ~Evaluator();

    void reset();

    QVariant evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid);

    bool getParseError() { return m_parseError; }

private:
    enum class TokenType
    {
        None                = 0,
        Abort               = -1,
        Symbol              = -2,
        FieldValue          = -3,
        Number              = -4,
        Text                = -5,
        End                 = -6,
        LessThanOrEqual     = -7,
        GreaterThanOrEqual  = -8,
        Equal               = -9,
        NotEqual            = -10,
        And                 = -11,
        Or                  = -12,
        AssignAdd           = -13,
        AssignSubtract      = -14,
        AssignMultiply      = -15,
        AssignDivide        = -16,
        Plus                = '+',
        Minus               = '-',
        Multiply            = '*',
        Divide              = '/',
        LeftBrace           = '(',
        RightBrace          = ')',
        ArrayStart          = '[',
        ArrayClose          = ']',
        Comma               = ',',
        Not                 = '!',
        LessThan            = '<',
        GreaterThan         = '>',
        Compare             = '=',
    };

    const QVariantMap* m_symbols = nullptr;
    QueryFieldValue m_queryFieldValue = nullptr;
    const QVariantMap* m_variables = nullptr;
    QString m_contextRecordUid;
    QString m_contextFieldUid;
    QString m_program;

    bool m_parseError;
    bool m_abort;
    const QChar* m_pword = nullptr;
    const QChar* m_pwordstart = nullptr;

    TokenType m_tokenType;
    QStringView m_word;
    QVariant m_value;

private:
    TokenType getToken(const bool ignoreSign = false);
    QVariant commaList(const bool Value);
    QVariant expression(const bool Value);
    QVariant comparison(const bool Value);
    QVariant addSubtract(const bool Value);
    QVariant term(const bool Value);
    QVariant primary(const bool Value);

    void checkToken(const TokenType Wanted);
    void parseFailure(const QString& message);
    bool compareVariants(const QVariant& v1, const QVariant& v2);
    double doubleVariant(const QVariant& v);
};

} // namespace CyberTracker
