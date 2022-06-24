/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public License Version 1.1         *
//   (the "License"); you may not use this file except in compliance with the License. You may    *
//   obtain a copy of the License from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the License for the specific language governing  *
//   rights and limitations under the License.                                                    *
//                                                                                                *
//************************************************************************************************/

#include "ctElementDate.h"
#include "fxApplication.h"
#include "fxMath.h"
#include "fxUtils.h"

CfxControl *Create_Control_ElementDate(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementDate(pOwner, pParent);
}

CctControl_ElementDate::CctControl_ElementDate(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTDATE);
    InitLockProperties("Result Element");

    InitBorder(bsSingle, 0);

    InitXGuid(&_elementId);

    InitFont(_titleFont, F_FONT_DEFAULT_LB);

    _year = 2000;
    _month = 1;
    _day = 1;

    _titleTextColor = 0xFFFFFF;
    _titleBackgroundColor = 0;
    _buttonTextColor = 0;
    _color = 0xFFFFFF;

    // Year
    _panelYearTitle = new CfxControl_Panel(pOwner, this);
    _panelYearTitle->SetColor(0);
    _panelYearTitle->SetTextColor(0xFFFFFF);
    _panelYearTitle->SetComposite(TRUE);
    _panelYearTitle->SetCaption("Year");
    _panelYearTitle->SetBorderStyle(bsNone);
    _panelYearTitle->SetBorderWidth(0);
    _panelYearTitle->SetHeight(20);

    _panelYear = new CfxControl_Panel(pOwner, this);
    _panelYear->SetComposite(TRUE);
    _panelYear->SetHeight(20);
    _panelYear->SetTransparent(TRUE);
    _panelYear->SetBorderStyle(bsNone);
  
    _backButton = new CfxControl_Button(pOwner, _panelYear);
    _backButton->SetComposite(TRUE);
    _backButton->SetBorderStyle(bsNone);
    _backButton->SetBorderWidth(0);
    _backButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementDate::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, _panelYear);
    _nextButton->SetComposite(TRUE);
    _nextButton->SetBorderStyle(bsNone);
    _nextButton->SetBorderWidth(0);
    _nextButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementDate::OnNextPaint);

     // Month
    _panelMonthTitle = new CfxControl_Panel(pOwner, this);
    _panelMonthTitle->SetColor(0);
    _panelMonthTitle->SetTextColor(0xFFFFFF);
    _panelMonthTitle->SetComposite(TRUE);
    _panelMonthTitle->SetCaption("Month");
    _panelMonthTitle->SetBorderStyle(bsNone);
    _panelMonthTitle->SetBorderWidth(0);
    _panelMonthTitle->SetHeight(20);

    _panelMonth = new CfxControl_Panel(pOwner, this);
    _panelMonth->SetComposite(TRUE);
    _panelMonth->SetHeight(20);
    _panelMonth->SetTransparent(TRUE);
    _panelMonth->SetBorderStyle(bsNone);

    for (UINT Month = 0; Month < 12; Month++)
    {
        _monthButtons[Month] = new CfxControl_Button(pOwner, _panelMonth);
        _monthButtons[Month]->SetTag(Month + 1);
        _monthButtons[Month]->SetComposite(TRUE);
        _monthButtons[Month]->SetBorderStyle(bsNone);
        _monthButtons[Month]->SetBorderWidth(0);
        _monthButtons[Month]->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnMonthClick);
        _monthButtons[Month]->SetGroupId(1);
        
        CHAR MonthBuffer[16];
        sprintf(MonthBuffer, "%ld", Month + 1);
        _monthButtons[Month]->SetCaption(MonthBuffer);
    }
    
     // Day
    _panelDayTitle = new CfxControl_Panel(pOwner, this);
    _panelDayTitle->SetColor(0);
    _panelDayTitle->SetTextColor(0xFFFFFF);
    _panelDayTitle->SetComposite(TRUE);
    _panelDayTitle->SetCaption("Day");
    _panelDayTitle->SetBorderStyle(bsNone);
    _panelDayTitle->SetBorderWidth(0);
    _panelDayTitle->SetHeight(20);

    _panelDay = new CfxControl_Panel(pOwner, this);
    _panelDay->SetComposite(TRUE);
    _panelDay->SetHeight(20*5);
    _panelDay->SetTransparent(TRUE);
    _panelDay->SetBorderStyle(bsNone);

    for (UINT Day = 0; Day < 31; Day++)
    {
        _dayButtons[Day] = new CfxControl_Button(pOwner, _panelDay);
        _dayButtons[Day]->SetTag(Day+1);
        _dayButtons[Day]->SetComposite(TRUE);
        _dayButtons[Day]->SetBorderStyle(bsNone);
        _dayButtons[Day]->SetBorderWidth(0);
        _dayButtons[Day]->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnDayClick);
        _dayButtons[Day]->SetGroupId(2);
        
        CHAR DayBuffer[16];
        sprintf(DayBuffer, "%ld", Day + 1);
        _dayButtons[Day]->SetCaption(DayBuffer);
    }

    // Today Button
    _todayButton = new CfxControl_Button(pOwner, _panelDay);
    _todayButton->SetComposite(TRUE);
    _todayButton->SetBorderStyle(bsNone);
    _todayButton->SetBorderWidth(0);
    _todayButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnTodayClick);
    _todayButton->SetCaption("Today");

    // Other
    if (IsLive()) 
    {
        FXDATETIME dt;
        GetHost(this)->GetDateTime(&dt);
        _year = dt.Year;
        _month = dt.Month;
        _day = dt.Day;
    }

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementDate::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementDate::OnSessionNext);
}

CctControl_ElementDate::~CctControl_ElementDate()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
}

VOID CctControl_ElementDate::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
   *((BOOL *)pParams) = TRUE;
}

VOID CctControl_ElementDate::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (IsNullXGuid(&_elementId)) 
    {
        return;
    }

    FXDATETIME dt;
    memset(&dt,0, sizeof(dt));
    
    dt.Day = _day;
    dt.Month = _month;
    dt.Year = _year;
    
    DOUBLE d = EncodeDateTime(&dt);

    CctAction_MergeAttribute action(this);
    action.SetControlId(_id);
    action.SetupAdd(_id, _elementId, dtDate, &d);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ElementDate::OnBackClick(CfxControl *pSender, VOID *pParams)
{
    _year = max(_year--, 2000);
    Update();
}

VOID CctControl_ElementDate::OnNextClick(CfxControl *pSender, VOID *pParams)
{
    _year++;
    Update();
}

VOID CctControl_ElementDate::OnMonthClick(CfxControl *pSender, VOID *pParams)
{
    _month = (UINT16)pSender->GetTag();
    Update();
}

VOID CctControl_ElementDate::OnDayClick(CfxControl *pSender, VOID *pParams)
{
    _day = (UINT16)pSender->GetTag();
    Update();
}

VOID CctControl_ElementDate::OnTodayClick(CfxControl *pSender, VOID *pParams)
{
    if (IsLive()) 
    {
        FXDATETIME dt;
        GetHost(this)->GetDateTime(&dt);
        _year = dt.Year;
        _month = dt.Month;
        _day = dt.Day;
    }

    Update();
}

VOID CctControl_ElementDate::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawBackArrow(pParams->Rect, _color, _buttonTextColor, _backButton->GetDown());
}

VOID CctControl_ElementDate::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawNextArrow(pParams->Rect, _color, _buttonTextColor, _nextButton->GetDown());
}

VOID CctControl_ElementDate::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("Element", &_elementId);
    F.DefineXFONT("TitleFont", &_titleFont, F_FONT_FIXED_L);
    F.DefineValue("TitleTextColor", dtColor, &_titleTextColor);
    F.DefineValue("TitleBackgroundColor", dtColor, &_titleBackgroundColor);
    F.DefineValue("ButtonTextColor", dtColor, &_buttonTextColor);

    if (F.IsReader())
    {
        Update();
    }
}

VOID CctControl_ElementDate::DefineState(CfxFiler &F)
{
    CfxControl::DefineState(F);
    F.DefineValue("Year", dtInt32, &_year);
    F.DefineValue("Month", dtInt32, &_month);
    F.DefineValue("Day", dtInt32, &_day);

    if (F.IsReader())
    {
        Update();
    }
}

VOID CctControl_ElementDate::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button text color", dtColor, &_buttonTextColor);
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Result Element", dtGuid,    &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Title background color", dtColor, &_titleBackgroundColor);
    F.DefineValue("Title text color", dtColor, &_titleTextColor);
    F.DefineValue("Title font",   dtFont,      &_titleFont);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

VOID CctControl_ElementDate::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtDate);
    F.DefineFont(_titleFont);
#endif
}

VOID CctControl_ElementDate::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    INT rowHeight = Height / 10;

    // Year
    _panelYearTitle->SetBounds(0, 0, Width, rowHeight);
    _panelYearTitle->AssignFont(&_titleFont);
    _panelYearTitle->SetTextColor(_titleTextColor);
    _panelYearTitle->SetColor(_titleBackgroundColor);
    _panelYear->SetBounds(0, rowHeight, Width, rowHeight);
    _panelYear->SetTextColor(_buttonTextColor);
    _panelYear->AssignFont(&_font);
    _backButton->SetBounds(0, 0, 20 * _oneSpace, rowHeight);
    _nextButton->SetBounds(Width - 20 * _oneSpace, 0, 20 * _oneSpace, rowHeight);

    // Month
    _panelMonthTitle->SetBounds(0, 2*rowHeight, Width, rowHeight);
    _panelMonthTitle->SetTextColor(_titleTextColor);
    _panelMonthTitle->SetColor(_titleBackgroundColor);
    _panelMonthTitle->AssignFont(&_titleFont);
    _panelMonth->SetBounds(0, 3*rowHeight, Width, rowHeight);
    
    for (INT Month = 0; Month < 12; Month++)
    {
        INT Padding = 0;//(Width % 12) / 2;
        DOUBLE ButtonWidth = (DOUBLE)Width / 12;
        DOUBLE Left = ButtonWidth * (DOUBLE)Month;

        if (Month == 11)
        {
            ButtonWidth = Width - (Padding + Left);
        }

        _monthButtons[Month]->SetBounds(Padding + (INT)Left, 0, (UINT)FxRound(ButtonWidth), rowHeight);
        _monthButtons[Month]->SetTextColor(_buttonTextColor);
        _monthButtons[Month]->SetColor(_color);
        _monthButtons[Month]->AssignFont(&_font);
    }

    // Day
    _panelDayTitle->SetBounds(0, 4*rowHeight, Width, rowHeight);
    _panelDayTitle->SetTextColor(_titleTextColor);
    _panelDayTitle->SetColor(_titleBackgroundColor);
    _panelDayTitle->AssignFont(&_titleFont);
    _panelDay->SetBounds(0, 5*rowHeight, Width, rowHeight*5);
    
    for (INT Day = 0; Day < 31; Day++)
    {
        INT ButtonWidth = Width / 7;
        INT Top = (Day / 7) * rowHeight;
        INT Left = ButtonWidth * ( Day % 7 );
        INT Padding = 0;

        if ((Day + 1) % 7 == 0) 
        {
            ButtonWidth = Width - (Padding + Left);
        }

        _dayButtons[Day]->SetBounds(Padding + Left, Top, ButtonWidth, rowHeight);
        _dayButtons[Day]->SetTextColor(_buttonTextColor);
        _dayButtons[Day]->SetColor(_color);
        _dayButtons[Day]->AssignFont(&_font);
    }

    // Today
    _todayButton->SetBounds(Width - Width / 2, 4 * rowHeight, Width / 2, rowHeight);
    _todayButton->SetTextColor(_buttonTextColor);
    _todayButton->SetColor(_color);
    _todayButton->AssignFont(&_font);
}

UINT16 GetNumberOfDays( UINT16 year, UINT16 month )
{
    UINT16 days = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11)
    {
        days = 30;
    }
    else if (month == 2)
    {
        BOOL leapyear = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        days = leapyear ? 29 : 28;
    }

    return days;   
}

VOID CctControl_ElementDate::Update()
{
    // Get number of day for given year and month
    UINT16 _daysInMonth = GetNumberOfDays(_year, _month);
    for (UINT i = 0; i < 31; i++)
    {
        _dayButtons[i]->SetVisible(i < _daysInMonth);
    }

    // Select smaller day if _day > _days in month
    if (_day > _daysInMonth)
    {
        _day = _daysInMonth;
    }

    CHAR Buffer[16];
    
    sprintf(Buffer, "%04d-%02d-%02d", _year, _month, _day);
    _panelYearTitle->SetCaption(Buffer);
    sprintf(Buffer, "%04d", _year );
    _panelYear->SetCaption(Buffer);
    _monthButtons[_month-1]->SetDown(TRUE);
    _dayButtons[_day-1]->SetDown(TRUE);

    Repaint();
}
