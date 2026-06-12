#include "fxParser.h"
#include "fxMath.h"
#include "fxUtils.h"

//*************************************************************************************************
// CfxString

CfxString::CfxString()
{
    _string = nullptr;
}

CfxString::CfxString(const char *pValue, int Count)
{
    if (pValue)
    {
        int size = ((Count == 0) ? (int) strlen(pValue) : Count) + 1;
        _string = (char *) MEM_ALLOC(size);
        memset(_string, 0, size);
        memcpy(_string, pValue, size-1);
    }
    else
    {
        _string = AllocString("");
    }
}

CfxString::~CfxString()
{
    MEM_FREE(_string);
    _string = nullptr;
}

char *CfxString::Get()
{
    return _string;
}

void CfxString::Set(const char *pValue, int Count)
{
    MEM_FREE(_string);
    _string = nullptr;

    if (pValue)
    {
        int size = ((Count == 0) ? (int) strlen(pValue) : Count) + 1;
        _string = (char *) MEM_ALLOC(size);
        memset(_string, 0, size);
        memcpy(_string, pValue, size-1);
    }
    else
    {
        _string = AllocString("");
    }
}

bool CfxString::Compare(const char *pValue)
{
    return stricmp(_string, pValue) == 0;
}

int CfxString::Length()
{
    return (int) strlen(_string);
}

//*************************************************************************************************
// Functions called from an expression

double DoInt(const double arg)
{
    return (int) arg;   
}

double DoAbs(const double arg)
{
    return FxAbs(arg);   
}

double DoSin(const double arg)
{
    return sin(DEG2RAD(arg));   
}

double DoASin(const double arg)
{
    return (asin(arg) * 180) / PI;   
}

double DoSinh(const double arg)
{
    return sinh(arg);   
}

double DoCos(const double arg)
{
    return cos(DEG2RAD(arg));   
}

double DoACos(const double arg)
{
    return (acos(arg) * 180.0) / PI;   
}

double DoCosh(const double arg)
{
    return cosh(arg);   
}

double DoTan(const double arg)
{
    return tan(DEG2RAD(arg));   
}

double DoATan(const double arg)
{
    return (atan(arg) * 180.0) / PI;   
}

double DoTanh(const double arg)
{
    return tanh(arg);   
}

double DoMin(const double arg1, const double arg2)
{
    return (arg1 < arg2 ? arg1 : arg2);
}

double DoMax(const double arg1, const double arg2)
{
    return (arg1 > arg2 ? arg1 : arg2);
}

double DoExp(const double arg)
{
    return exp(arg);
}

double DoCeil(const double arg)
{
    return ceil(arg);
}

double DoFloor(const double arg)
{
    return floor(arg);
}

double DoLog(const double arg)
{
    return log(arg);
}

double DoSqrt(const double arg)
{
    return sqrt(arg);
}

/*double DoFmod(const double arg1, const double arg2)
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
double DoPow(const double arg1, const double arg2)
{
    return pow(arg1, arg2);
}

double DoIf(const double arg1, const double arg2, const double arg3)
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

double DoDate(const double arg1, const double arg2, const double arg3)
{
    FXDATETIME dateTime = {0};
    dateTime.Year = (UINT16)arg1;
    dateTime.Month = (UINT16)arg2;
    dateTime.Day = (UINT16)arg3;

    return EncodeDateTime(&dateTime);
}

typedef double (*OneArgFunction) (const double arg);
struct FUNCTIONTABLE1
{
    const char *Name = nullptr;
    OneArgFunction Fn = nullptr;
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

typedef double (*TwoArgFunction) (const double arg1, const double arg2);
struct FUNCTIONTABLE2
{
    const char *Name = nullptr;
    TwoArgFunction Fn = nullptr;
};

typedef double (*ThreeArgFunction) (const double arg1, const double arg2, const double arg3);
FUNCTIONTABLE2 FunctionTable2[] =
{
    { "min",   DoMin  },
    { "max",   DoMax  },
    { "pow",   DoPow  }
//    { "mod",   DoFmod },
};

struct FUNCTIONTABLE3
{
    const char *Name = nullptr;
    ThreeArgFunction Fn = nullptr;
};

FUNCTIONTABLE3 FunctionTable3[] =
{
    { "if",    DoIf   },
    { "date",  DoDate }
};

//*************************************************************************************************
// CfxExpressionParser

CfxExpressionParser::CfxExpressionParser(char *pExpression): _program(pExpression), _tokenType (NONE), _symbolNames(), _symbolValues()
{
    _parseError = false;
    _pword = _program.Get();

    // Insert pre-defined names
    SetSymbol("pi", 3.1415926535897932385);
    SetSymbol("e", 2.7182818284590452354);
}

CfxExpressionParser::~CfxExpressionParser()
{
    for (int i=0; i<_symbolNames.GetCount(); i++)
    {
        MEM_FREE(_symbolNames.Get(i));
    }
    _symbolNames.Clear();
    _symbolValues.Clear();
}

bool CfxExpressionParser::GetSymbol(const char *pSymbolName, double *pValue)
{
    for (int i=0; i<_symbolNames.GetCount(); i++)
    {
        if (strcmp(_symbolNames.Get(i), pSymbolName) == 0)
        {
            *pValue = _symbolValues.Get(i);
            return true;
        }
    }

    return false;
}

void CfxExpressionParser::SetSymbol(const char *pSymbolName, const double Value)
{
   for (int i=0; i<_symbolNames.GetCount(); i++)
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

CfxExpressionParser::TokenType CfxExpressionParser::GetToken(const bool ignoreSign)
{
    if (_parseError) return _tokenType = END;

    // Clear the current word
    _word.Set(nullptr);

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
        _parseError = true;
        return _tokenType = END;
    }

    // First character in new _word
    char firstChar = *_pword;        
    if (firstChar == 0)    // stop at end of file
    {
        _word.Set("<end>");
        return _tokenType = END;
    }

    // Second character in new _word
    char nextChar = *(_pword + 1);  

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

        // Get double value
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
        _parseError = true;
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

double CfxExpressionParser::Primary(const bool Value)   // primary (base) tokens
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
            double v = _value; 
            GetToken(true);  // get next one (one-token lookahead)
            return v;
        }

    case NAME:
        {
            CfxString word(_word.Get());
            GetToken(true); 
            if (_tokenType == LHPAREN)
            {
                int i;

                // Single arg
                for (i=0; i<ARRAYSIZE(FunctionTable1); i++)
                {
                    if (word.Compare(FunctionTable1[i].Name))
                    {
                        double v = Expression (true);   // get argument
                        CheckToken (RHPAREN);
                        GetToken (true);        // get next one (one-token lookahead)
                        return FunctionTable1[i].Fn(v);  // evaluate function
                    }
                }

                // Double arg
                for (i=0; i<ARRAYSIZE(FunctionTable2); i++)
                {
                    if (word.Compare(FunctionTable2[i].Name))
                    {
                        double v1 = Expression (true);   // get argument 1 (not commalist)
                        CheckToken (COMMA);
                        double v2 = Expression (true);   // get argument 2 (not commalist)
                        CheckToken (RHPAREN);
                        GetToken (true);            // get next one (one-token lookahead)
                        return FunctionTable2[i].Fn(v1, v2); // evaluate function
                    }
                }

                // Triple arg
                for (i=0; i<ARRAYSIZE(FunctionTable3); i++)
                {
                    if (word.Compare(FunctionTable3[i].Name))
                    {
                        double v1 = Expression (true);   // Get argument 1 (not commalist)
                        CheckToken (COMMA);
                        double v2 = Expression (true);   // Get argument 2 (not commalist)
                        CheckToken (COMMA);
                        double v3 = Expression (true);   // Get argument 3 (not commalist)
                        CheckToken (RHPAREN);
                        GetToken (true);                 // Get next one (one-token lookahead)
                        return FunctionTable3[i].Fn(v1, v2, v3); // evaluate function
                    }
                }

                _parseError = true;
                return 0;
            }

            // Must be a symbol in the symbol table
            double v = 0;
            GetSymbol(word.Get(), &v);

            switch (_tokenType)
            {
            case ASSIGN:     v  = Expression (true); break;   
            case ASSIGN_ADD: v += Expression (true); break;   
            case ASSIGN_SUB: v -= Expression (true); break;   
            case ASSIGN_MUL: v *= Expression (true); break;   
            case ASSIGN_DIV: 
                {
                    double d = Expression (true); 
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
        return - Primary (true);

    case NOT:   
        return (Primary (true) == 0.0) ? 1.0 : 0.0;;

    case LHPAREN:
        {
            double v = CommaList(true);
            CheckToken (RHPAREN);
            GetToken();
            return v;
        }

    default:   
        _parseError = true;
        return 0;
    } 
} 

double CfxExpressionParser::Term(const bool Value)
{
    double left = Primary(Value);
    while (true)
    {
        switch (_tokenType)
        {
        case MULTIPLY:  
            left *= Primary(true); 
            break;
        case DIVIDE: 
            {
                double d = Primary(true);
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

double CfxExpressionParser::AddSubtract(const bool Value)  // add and subtract
{
    double left = Term(Value);
    while (true)
    {
        switch (_tokenType)
        {
        case PLUS:  left += Term(true); break;
        case MINUS: left -= Term(true); break;
        default:    
            return left;
        }
    }  
} 

double CfxExpressionParser::Comparison(const bool Value)
{
    double left = AddSubtract(Value);
    while (true)
    {
        switch (_tokenType)
        {
        case LT: left = left <  AddSubtract (true) ? 1.0 : 0.0; break;
        case GT: left = left >  AddSubtract (true) ? 1.0 : 0.0; break;
        case LE: left = left <= AddSubtract (true) ? 1.0 : 0.0; break;
        case GE: left = left >= AddSubtract (true) ? 1.0 : 0.0; break;
        case EQ: left = left == AddSubtract (true) ? 1.0 : 0.0; break;
        case NE: left = left != AddSubtract (true) ? 1.0 : 0.0; break;
        default:    
            return left;
        } 
    }   
} 

double CfxExpressionParser::Expression(const bool Value)  
{
    double left = Comparison(Value);
    while (true)
    {
        switch (_tokenType)
        {
        case AND: 
            {
                // Don't want short-circuit evaluation
                double d = Comparison(true);   
                left = (left != 0.0) && (d != 0.0); 
            }
            break;
        case OR:  
            {
                // Don't want short-circuit evaluation
                double d = Comparison(true);   
                left = (left != 0.0) || (d != 0.0); 
            }
            break;
        default:    
            return left;
        } 
    }
} 

double CfxExpressionParser::CommaList(const bool Value)
{
    double left = Expression(Value);
    while (true)
    {
        switch (_tokenType)
        {
        case COMMA: 
            left = Expression (true); 
            break; 
        default:    
            return left;
        } 
    }   
} 

double CfxExpressionParser::Evaluate()  
{
    double v = CommaList(true);
    if (_tokenType != END)
    {
        _parseError = true;
        return 0;
    }
    else
    {
        return v;  
    }
}

double CfxExpressionParser::Evaluate(const char *pExpression)  
{
    _program.Set(pExpression);
    _pword = _program.Get();
    _tokenType = NONE;
    return Evaluate();
}

