#include "fxMath.h"

int FxAbs(int Value)
{
    if (Value < 0)
    {
        return -Value;
    }
    else
    {
        return Value;
    }
}

double FxAbs(double Value)
{
    if (Value < 0)
    {
        return -Value;
    }
    else
    {
        return Value;
    }
}

int FxRound(double Value)
{
    return (int)(Value + 0.5);
}

int FxRound(float Value)
{
    return (int)(Value + 0.5);
}