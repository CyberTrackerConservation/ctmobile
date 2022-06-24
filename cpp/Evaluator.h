#pragma once
#include "pch.h"

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
        None,
        Abort,
        Symbol,
        FieldValue,
        Number,
        Text,
        End,
        LessThanOrEqual,
        GreaterThanOrEqual,
        Equal,
        NotEqual,
        And,
        Or,
        Plus = '+',
        Minus = '-',
        Multiply = '*',
        Divide = '/',
        LeftBrace = '(',
        RightBrace = ')',
        ArrayStart = '[',
        ArrayClose = ']',
        Comma = ',',
        Not = '!',
        LessThan = '<',
        GreaterThan = '>',
        Compare = '=',
        AssignAdd,
        AssignSubtract,
        AssignMultiply,
        AssignDivide
    };

    const QVariantMap* m_symbols = nullptr;
    QueryFieldValue m_queryFieldValue = nullptr;
    const QVariantMap* m_variables = nullptr;
    QString m_contextRecordUid;
    QString m_contextFieldUid;
    QByteArray m_program;

    bool m_parseError;
    bool m_abort;
    const char* m_pword;
    const char* m_pwordstart;

    TokenType m_tokenType;
    QString m_word;
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


