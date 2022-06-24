#pragma once
#include "pch.h"
#include "fxClasses.h"

class CfxExpressionParser
{
public:
    enum TokenType { NONE, NAME, NUMBER, END, PLUS='+', MINUS='-', MULTIPLY='*', DIVIDE='/', ASSIGN='=',
        LHPAREN='(', RHPAREN=')', COMMA=',', NOT='!', LT='<', GT='>', LE, GE, EQ, NE, AND, OR,   
        ASSIGN_ADD, ASSIGN_SUB, ASSIGN_MUL, ASSIGN_DIV};
private:
    TList<CHAR *> _symbolNames;
    TList<DOUBLE> _symbolValues;
    CfxString _program;

    BOOL _parseError;
    CHAR * _pword;
    CHAR * _pwordstart;

    TokenType _tokenType;
    CfxString _word;
    DOUBLE _value;
     
public:
    CfxExpressionParser(CHAR *pExpression);
    ~CfxExpressionParser();
  
    DOUBLE Evaluate ();  
    DOUBLE Evaluate(const CHAR *pExpression);  

    BOOL GetSymbol(const CHAR *pSymbolName, DOUBLE *pValue);
    VOID SetSymbol(const CHAR *pSymbolName, const DOUBLE Value);

    BOOL GetParseError() { return _parseError; }

private:
    TokenType GetToken(const BOOL ignoreSign = FALSE);
    DOUBLE CommaList(const BOOL Value);
    DOUBLE Expression(const BOOL Value);
    DOUBLE Comparison(const BOOL Value);
    DOUBLE AddSubtract(const BOOL Value);
    DOUBLE Term(const BOOL Value);      
    DOUBLE Primary(const BOOL Value);   

    inline VOID CheckToken (const TokenType Wanted)
    {
        if (_tokenType != Wanted)
        {
            _parseError = true;
        }
    }
};  


