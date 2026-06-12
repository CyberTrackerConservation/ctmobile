#pragma once
#include "pch.h"
#include "fxClasses.h"

// CfxString: retained here for CfxExpressionParser's internal pointer arithmetic.
class CfxString
{
protected:
    char *_string = nullptr;
public:
    CfxString();
    CfxString(const char *pValue, int Count=0);
    ~CfxString();
    char *Get();
    void Set(const char *pValue, int Count=0);
    bool Compare(const char *pValue);
    int Length();
};

class CfxExpressionParser
{
public:
    enum TokenType { NONE, NAME, NUMBER, END, PLUS='+', MINUS='-', MULTIPLY='*', DIVIDE='/', ASSIGN='=',
        LHPAREN='(', RHPAREN=')', COMMA=',', NOT='!', LT='<', GT='>', LE, GE, EQ, NE, AND, OR,   
        ASSIGN_ADD, ASSIGN_SUB, ASSIGN_MUL, ASSIGN_DIV};
private:
    TList<char *> _symbolNames;
    TList<double> _symbolValues;
    CfxString _program;

    bool _parseError = false;
    char * _pword = nullptr;
    char * _pwordstart = nullptr;

    TokenType _tokenType = {};
    CfxString _word;
    double _value = 0.0;
     
public:
    CfxExpressionParser(char *pExpression);
    ~CfxExpressionParser();
  
    double Evaluate ();  
    double Evaluate(const char *pExpression);  

    bool GetSymbol(const char *pSymbolName, double *pValue);
    void SetSymbol(const char *pSymbolName, const double Value);

    bool GetParseError() { return _parseError; }

private:
    TokenType GetToken(const bool ignoreSign = false);
    double CommaList(const bool Value);
    double Expression(const bool Value);
    double Comparison(const bool Value);
    double AddSubtract(const bool Value);
    double Term(const bool Value);      
    double Primary(const bool Value);   

    inline void CheckToken (const TokenType Wanted)
    {
        if (_tokenType != Wanted)
        {
            _parseError = true;
        }
    }
};  


