#pragma once
#include "pch.h"
#include "fxTypes.h"

#ifndef DEG2RAD
#define DEG2RAD(d) ((d * PI) / 180.0)
#endif

#ifndef RAD2DEG
#define RAD2DEG(r) ((r * 180.0) / PI)
#endif

int FxAbs(int Value);
double FxAbs(double Value);
int FxRound(double Value);
int FxRound(float Value);
