#include "fxParser.h"
#include "fxMath.h"
#include "fxUtils.h"

//*************************************************************************************************
// Functions called from an expression

DOUBLE DoInt(const DOUBLE arg)
{
    return (INT) arg;   
}

DOUBLE DoAbs(const DOUBLE arg)
{
    return FxAbs(arg);   
}

DOUBLE DoSin(const DOUBLE arg)
{
    return sin(DEG2RAD(arg));   
}

DOUBLE DoASin(const DOUBLE arg)
{
    return (asin(arg) * 180) / PI;   
}

DOUBLE DoSinh(const DOUBLE arg)
{
    return sinh(arg);   
}

DOUBLE DoCos(const DOUBLE arg)
{
    return cos(DEG2RAD(arg));   
}

DOUBLE DoACos(const DOUBLE arg)
{
    return (acos(arg) * 180.0) / PI;   
}

DOUBLE DoCosh(const DOUBLE arg)
{
    return cosh(arg);   
}

DOUBLE DoTan(const DOUBLE arg)
{
    return tan(DEG2RAD(arg));   
}

DOUBLE DoATan(const DOUBLE arg)
{
    return (atan(arg) * 180.0) / PI;   
}

DOUBLE DoTanh(const DOUBLE arg)
{
    return tanh(arg);   
}

DOUBLE DoMin(const DOUBLE arg1, const DOUBLE arg2)
{
    return (arg1 < arg2 ? arg1 : arg2);
}

DOUBLE DoMax(const DOUBLE arg1, const DOUBLE arg2)
{
    return (arg1 > arg2 ? arg1 : arg2);
}

DOUBLE DoExp(const DOUBLE arg)
{
    return exp(arg);
}

DOUBLE DoCeil(const DOUBLE arg)
{
    return ceil(arg);
}

DOUBLE DoFloor(const DOUBLE arg)
{
    return floor(arg);
}

DOUBLE DoLog(const DOUBLE arg)
{
    return log(arg);
}

DOUBLE DoSqrt(const DOUBLE arg)
{
    return sqrt(arg);
}

/*DOUBLE DoFmod(const DOUBLE arg1, const DOUBLE arg2)
{
    if (arg2 == 0.0)
    {
        return 0.0;
    }
    else
    {
        return fmod (arg1, arg2);
    }
}
*/
DOUBLE DoPow(const DOUBLE arg1, const DOUBLE arg2)
{
    return pow(arg1, arg2);
}

DOUBLE DoIf(const DOUBLE arg1, const DOUBLE arg2, const DOUBLE arg3)
{
    if (arg1 != 0.0)
    {
        return arg2;
    }
    else
    {
        return arg3;
    }
}

DOUBLE DoDate(const DOUBLE arg1, const DOUBLE arg2, const DOUBLE arg3)
{
    FXDATETIME dateTime = {0};
    dateTime.Year = (UINT16)arg1;
    dateTime.Month = (UINT16)arg2;
    dateTime.Day = (UINT16)arg3;

    return EncodeDateTime(&dateTime);
}

typedef DOUBLE (*OneArgFunction) (const DOUBLE arg);
struct FUNCTIONTABLE1
{
    const CHAR *Name;
    OneArgFunction Fn;
};
FUNCTIONTABLE1 FunctionTable1[] = 
{
    { "cos",   DoCos  },
    { "acos",  DoACos },
    { "cosh",  DoCosh },

    { "sin",   DoSin  },
    { "asin",  DoASin },
    { "sinh",  DoSinh },

    { "tan",   DoTan  },
    { "atan",  DoATan },
    { "tanh",  DoTanh },

    { "ceil",  DoCeil },
    { "floor", DoFloor },
    { "log",   DoLog  },
    { "sqrt",  DoSqrt   },

    { "exp",   DoExp  },
    { "abs",   DoAbs  },
    { "int",   DoInt  }
};

typedef DOUBLE (*TwoArgFunction) (const DOUBLE arg1, const DOUBLE arg2);
struct FUNCTIONTABLE2
{
    const CHAR *Name;
    TwoArgFunction Fn;
};

typedef DOUBLE (*ThreeArgFunction) (const DOUBLE arg1, const DOUBLE arg2, const DOUBLE arg3);
FUNCTIONTABLE2 FunctionTable2[] =
{
    { "min",   DoMin  },
    { "max",   DoMax  },
    { "pow",   DoPow  }
//    { "mod",   DoFmod },
};

struct FUNCTIONTABLE3
{
    const CHAR *Name;
    ThreeArgFunction Fn;
};

FUNCTIONTABLE3 FunctionTable3[] =
{
    { "if",    DoIf   },
    { "date",  DoDate }
};

//*************************************************************************************************
// CfxExpressionParser

CfxExpressionParser::CfxExpressionParser(CHAR *pExpression): _program(pExpression), _tokenType (NONE), _symbolNames(), _symbolValues()
{
    _parseError = FALSE;
    _pword = _program.Get();

    // Insert pre-defined names
    SetSymbol("pi", 3.1415926535897932385);
    SetSymbol("e", 2.7182818284590452354);
}

CfxExpressionParser::~CfxExpressionParser()
{
    for (UINT i=0; i<_symbolNames.GetCount(); i++)
    {
        MEM_FREE(_symbolNames.Get(i));
    }
    _symbolNames.Clear();
    _symbolValues.Clear();
}

BOOL CfxExpressionParser::GetSymbol(const CHAR *pSymbolName, DOUBLE *pValue)
{
    for (UINT i=0; i<_symbolNames.GetCount(); i++)
    {
        if (strcmp(_symbolNames.Get(i), pSymbolName) == 0)
        {
            *pValue = _symbolValues.Get(i);
            return TRUE;
        }
    }

    return FALSE;
}

VOID CfxExpressionParser::SetSymbol(const CHAR *pSymbolName, const DOUBLE Value)
{
   for (UINT i=0; i<_symbolNames.GetCount(); i++)
    {
        if (strcmp(_symbolNames.Get(i), pSymbolName) == 0)
        {
            _symbolValues.Put(i, Value);
            return;
        }
    }
    _symbolNames.Add(AllocString(pSymbolName));
    _symbolValues.Add(Value);
}

CfxExpressionParser::TokenType CfxExpressionParser::GetToken(const BOOL ignoreSign)
{
    if (_parseError) return _tokenType = END;

    // Clear the current word
    _word.Set(NULL);

    // Skip spaces
    while (*_pword && isspace(*_pword))
    {
        ++_pword;
    }

    // Remember current starting position
    _pwordstart = _pword;   

    // Check for unterminated statements
    if ((*_pword == 0) && (_tokenType == END))
    {
        _parseError = TRUE;
        return _tokenType = END;
    }

    // First character in new _word
    CHAR firstChar = *_pword;        
    if (firstChar == 0)    // stop at end of file
    {
        _word.Set("<end>");
        return _tokenType = END;
    }

    // Second character in new _word
    CHAR nextChar = *(_pword + 1);  

    // Look for number
    if ((!ignoreSign && 
        (firstChar == '+' || firstChar == '-') && 
        isdigit(nextChar)
        ) 
        || isdigit (firstChar))
    {
        // Skip sign
        if ((firstChar == '+' || firstChar == '-'))
        {
            _pword++;
        }

        while (isdigit (*_pword) || *_pword == '.')
        {
            _pword++;
        }

        // Allow for 1.01e+12
        if (*_pword == 'e' || *_pword == 'E')
        {
            // Skip 'e'
            _pword++; 
            if ((*_pword  == '+' || *_pword  == '-'))
            {
                // Skip sign after e
                _pword++; 
            }

            // Now digits after e
            while (isdigit (*_pword))  
            {
                _pword++;      
            }
        }

        _word.Set(_pwordstart, _pword - _pwordstart);

        // Get DOUBLE value
        _value = atof(_word.Get());

        return (_tokenType = NUMBER);
    }

    // Special test for 2-character sequences: <=, >=, ==, !=, +=, -=, /=, *=
    if (nextChar == '=')
    {
        switch (firstChar)
        {
        // Comparisons
        case '=': _tokenType = EQ; break;
        case '<': _tokenType = LE; break;
        case '>': _tokenType = GE; break;
        case '!': _tokenType = NE; break;
        // Assignments
        case '+': _tokenType = ASSIGN_ADD; break;
        case '-': _tokenType = ASSIGN_SUB; break;
        case '*': _tokenType = ASSIGN_MUL; break;
        case '/': _tokenType = ASSIGN_DIV; break;
        // None of the above
        default:  
            _tokenType = NONE; 
            break;
        } 

        if (_tokenType != NONE)
        {
            // Skip both characters
            _word.Set(_pwordstart, 2);
            _pword += 2;   
            return _tokenType;
        } 
    } 

    switch (firstChar)
    {
    case '&': 
        if (nextChar == '&')    // &&
        {
            _word.Set(_pwordstart, 2);
            _pword += 2;   // skip both characters
            return _tokenType = AND;
        }
        break;
    case '|': 
        if (nextChar == '|')   // ||
        {
            _word.Set(_pwordstart, 2);
            _pword += 2;   // skip both characters
            return _tokenType = OR;
        }
        break;
    // Single-character symboles
    case '=':
    case '<':
    case '>':
    case '+':
    case '-':
    case '/':
    case '*':
    case '(':
    case ')':
    case ',':
    case '!':
        _word.Set(_pwordstart, 1);
        ++_pword;   // skip it
        return _tokenType = TokenType(firstChar);
    } 

    if (!isalpha(firstChar))
    {
        _parseError = TRUE;
        return _tokenType = END;
    }

    // We have a word (starting with A-Z) - pull it out
    while (isalnum (*_pword) || *_pword == '_')
    {
        ++_pword;
    }

    _word.Set(_pwordstart, _pword - _pwordstart);

    return _tokenType = NAME;
}   

DOUBLE CfxExpressionParser::Primary(const BOOL Value)   // primary (base) tokens
{
    if (Value)
    {
        // Lookahead  
        GetToken();  
    }

    switch (_tokenType)
    {
    case NUMBER:  
        {
            DOUBLE v = _value; 
            GetToken(TRUE);  // get next one (one-token lookahead)
            return v;
        }

    case NAME:
        {
            CfxString word(_word.Get());
            GetToken(TRUE); 
            if (_tokenType == LHPAREN)
            {
                UINT i;

                // Single arg
                for (i=0; i<ARRAYSIZE(FunctionTable1); i++)
                {
                    if (word.Compare(FunctionTable1[i].Name))
                    {
                        DOUBLE v = Expression (TRUE);   // get argument
                        CheckToken (RHPAREN);
                        GetToken (TRUE);        // get next one (one-token lookahead)
                        return FunctionTable1[i].Fn(v);  // evaluate function
                    }
                }

                // Double arg
                for (i=0; i<ARRAYSIZE(FunctionTable2); i++)
                {
                    if (word.Compare(FunctionTable2[i].Name))
                    {
                        DOUBLE v1 = Expression (TRUE);   // get argument 1 (not commalist)
                        CheckToken (COMMA);
                        DOUBLE v2 = Expression (TRUE);   // get argument 2 (not commalist)
                        CheckToken (RHPAREN);
                        GetToken (TRUE);            // get next one (one-token lookahead)
                        return FunctionTable2[i].Fn(v1, v2); // evaluate function
                    }
                }

                // Triple arg
                for (i=0; i<ARRAYSIZE(FunctionTable3); i++)
                {
                    if (word.Compare(FunctionTable3[i].Name))
                    {
                        DOUBLE v1 = Expression (TRUE);   // Get argument 1 (not commalist)
                        CheckToken (COMMA);
                        DOUBLE v2 = Expression (TRUE);   // Get argument 2 (not commalist)
                        CheckToken (COMMA);
                        DOUBLE v3 = Expression (TRUE);   // Get argument 3 (not commalist)
                        CheckToken (RHPAREN);
                        GetToken (TRUE);                 // Get next one (one-token lookahead)
                        return FunctionTable3[i].Fn(v1, v2, v3); // evaluate function
                    }
                }

                _parseError = TRUE;
                return 0;
            }

            // Must be a symbol in the symbol table
            DOUBLE v = 0;
            GetSymbol(word.Get(), &v);

            switch (_tokenType)
            {
            case ASSIGN:     v  = Expression (TRUE); break;   
            case ASSIGN_ADD: v += Expression (TRUE); break;   
            case ASSIGN_SUB: v -= Expression (TRUE); break;   
            case ASSIGN_MUL: v *= Expression (TRUE); break;   
            case ASSIGN_DIV: 
                {
                    DOUBLE d = Expression (TRUE); 
                    if (d == 0.0)
                    {
                        // Divide by zero
                        v = 0;
                    }
                    else
                    {
                        v /= d;
                    }
                    break;
                } 
            default: break;   
            } 
            return v;
        }

    case MINUS: 
        return - Primary (TRUE);

    case NOT:   
        return (Primary (TRUE) == 0.0) ? 1.0 : 0.0;;

    case LHPAREN:
        {
            DOUBLE v = CommaList(TRUE);
            CheckToken (RHPAREN);
            GetToken();
            return v;
        }

    default:   
        _parseError = TRUE;
        return 0;
    } 
} 

DOUBLE CfxExpressionParser::Term(const BOOL Value)
{
    DOUBLE left = Primary(Value);
    while (TRUE)
    {
        switch (_tokenType)
        {
        case MULTIPLY:  
            left *= Primary(TRUE); 
            break;
        case DIVIDE: 
            {
                DOUBLE d = Primary(TRUE);
                if (d == 0.0)
                {
                    left = 0;
                }
                else
                {
                    left /= d; 
                }
                break;
            }
        default:    
            return left;
        }
    }   
} 

DOUBLE CfxExpressionParser::AddSubtract(const BOOL Value)  // add and subtract
{
    DOUBLE left = Term(Value);
    while (TRUE)
    {
        switch (_tokenType)
        {
        case PLUS:  left += Term(TRUE); break;
        case MINUS: left -= Term(TRUE); break;
        default:    
            return left;
        }
    }  
} 

DOUBLE CfxExpressionParser::Comparison(const BOOL Value)
{
    DOUBLE left = AddSubtract(Value);
    while (TRUE)
    {
        switch (_tokenType)
        {
        case LT: left = left <  AddSubtract (TRUE) ? 1.0 : 0.0; break;
        case GT: left = left >  AddSubtract (TRUE) ? 1.0 : 0.0; break;
        case LE: left = left <= AddSubtract (TRUE) ? 1.0 : 0.0; break;
        case GE: left = left >= AddSubtract (TRUE) ? 1.0 : 0.0; break;
        case EQ: left = left == AddSubtract (TRUE) ? 1.0 : 0.0; break;
        case NE: left = left != AddSubtract (TRUE) ? 1.0 : 0.0; break;
        default:    
            return left;
        } 
    }   
} 

DOUBLE CfxExpressionParser::Expression(const BOOL Value)  
{
    DOUBLE left = Comparison(Value);
    while (TRUE)
    {
        switch (_tokenType)
        {
        case AND: 
            {
                // Don't want short-circuit evaluation
                DOUBLE d = Comparison(TRUE);   
                left = (left != 0.0) && (d != 0.0); 
            }
            break;
        case OR:  
            {
                // Don't want short-circuit evaluation
                DOUBLE d = Comparison(TRUE);   
                left = (left != 0.0) || (d != 0.0); 
            }
            break;
        default:    
            return left;
        } 
    }
} 

DOUBLE CfxExpressionParser::CommaList(const BOOL Value)
{
    DOUBLE left = Expression(Value);
    while (TRUE)
    {
        switch (_tokenType)
        {
        case COMMA: 
            left = Expression (TRUE); 
            break; 
        default:    
            return left;
        } 
    }   
} 

DOUBLE CfxExpressionParser::Evaluate()  
{
    DOUBLE v = CommaList(TRUE);
    if (_tokenType != END)
    {
        _parseError = TRUE;
        return 0;
    }
    else
    {
        return v;  
    }
}

DOUBLE CfxExpressionParser::Evaluate(const CHAR *pExpression)  
{
    _program.Set(pExpression);
    _pword = _program.Get();
    _tokenType = NONE;
    return Evaluate();
}

