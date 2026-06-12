#include "ctElementDate.h"
#include "fxApplication.h"
#include "fxMath.h"
#include "fxUtils.h"
#include "fxTranslate.h"

CfxControl *Create_Control_ElementDate(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementDate(pOwner, pParent);
}

CctControl_ElementDate::CctControl_ElementDate(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTDATE);
    InitLockProperties("Result Element");

    InitBorder(ControlBorderStyle::Single, 0);

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
    _panelYearTitle->SetComposite(true);
    _panelYearTitle->SetCaption(fxTranslate("Year"));
    _panelYearTitle->SetBorderStyle(ControlBorderStyle::None);
    _panelYearTitle->SetBorderWidth(0);
    _panelYearTitle->SetHeight(20);

    _panelYear = new CfxControl_Panel(pOwner, this);
    _panelYear->SetComposite(true);
    _panelYear->SetHeight(20);
    _panelYear->SetTransparent(true);
    _panelYear->SetBorderStyle(ControlBorderStyle::None);
  
    _backButton = new CfxControl_Button(pOwner, _panelYear);
    _backButton->SetComposite(true);
    _backButton->SetBorderStyle(ControlBorderStyle::None);
    _backButton->SetBorderWidth(0);
    _backButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementDate::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, _panelYear);
    _nextButton->SetComposite(true);
    _nextButton->SetBorderStyle(ControlBorderStyle::None);
    _nextButton->SetBorderWidth(0);
    _nextButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementDate::OnNextPaint);

     // Month
    _panelMonthTitle = new CfxControl_Panel(pOwner, this);
    _panelMonthTitle->SetColor(0);
    _panelMonthTitle->SetTextColor(0xFFFFFF);
    _panelMonthTitle->SetComposite(true);
    _panelMonthTitle->SetCaption(fxTranslate("Month"));
    _panelMonthTitle->SetBorderStyle(ControlBorderStyle::None);
    _panelMonthTitle->SetBorderWidth(0);
    _panelMonthTitle->SetHeight(20);

    _panelMonth = new CfxControl_Panel(pOwner, this);
    _panelMonth->SetComposite(true);
    _panelMonth->SetHeight(20);
    _panelMonth->SetTransparent(true);
    _panelMonth->SetBorderStyle(ControlBorderStyle::None);

    for (int Month = 0; Month < 12; Month++)
    {
        _monthButtons[Month] = new CfxControl_Button(pOwner, _panelMonth);
        _monthButtons[Month]->SetTag(Month + 1);
        _monthButtons[Month]->SetComposite(true);
        _monthButtons[Month]->SetBorderStyle(ControlBorderStyle::None);
        _monthButtons[Month]->SetBorderWidth(0);
        _monthButtons[Month]->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnMonthClick);
        _monthButtons[Month]->SetGroupId(1);
        
        char MonthBuffer[16];
        sprintf(MonthBuffer, "%ld", Month + 1);
        _monthButtons[Month]->SetCaption(MonthBuffer);
    }
    
     // Day
    _panelDayTitle = new CfxControl_Panel(pOwner, this);
    _panelDayTitle->SetColor(0);
    _panelDayTitle->SetTextColor(0xFFFFFF);
    _panelDayTitle->SetComposite(true);
    _panelDayTitle->SetCaption(fxTranslate("Day"));
    _panelDayTitle->SetBorderStyle(ControlBorderStyle::None);
    _panelDayTitle->SetBorderWidth(0);
    _panelDayTitle->SetHeight(20);

    _panelDay = new CfxControl_Panel(pOwner, this);
    _panelDay->SetComposite(true);
    _panelDay->SetHeight(20*5);
    _panelDay->SetTransparent(true);
    _panelDay->SetBorderStyle(ControlBorderStyle::None);

    for (int Day = 0; Day < 31; Day++)
    {
        _dayButtons[Day] = new CfxControl_Button(pOwner, _panelDay);
        _dayButtons[Day]->SetTag(Day+1);
        _dayButtons[Day]->SetComposite(true);
        _dayButtons[Day]->SetBorderStyle(ControlBorderStyle::None);
        _dayButtons[Day]->SetBorderWidth(0);
        _dayButtons[Day]->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnDayClick);
        _dayButtons[Day]->SetGroupId(2);
        
        char DayBuffer[16];
        sprintf(DayBuffer, "%ld", Day + 1);
        _dayButtons[Day]->SetCaption(DayBuffer);
    }

    // Today Button
    _todayButton = new CfxControl_Button(pOwner, _panelDay);
    _todayButton->SetComposite(true);
    _todayButton->SetBorderStyle(ControlBorderStyle::None);
    _todayButton->SetBorderWidth(0);
    _todayButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementDate::OnTodayClick);
    _todayButton->SetCaption(fxTranslate("Today"));

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

void CctControl_ElementDate::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
   *((bool *)pParams) = true;
}

void CctControl_ElementDate::OnSessionNext(CfxControl *pSender, void *pParams)
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
    
    double d = EncodeDateTime(&dt);

    CctAction_MergeAttribute action(this);
    action.SetControlId(_id);
    action.SetupAdd(_id, _elementId, dtDate, &d);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_ElementDate::OnBackClick(CfxControl *pSender, void *pParams)
{
    _year = max(_year--, 2000);
    Update();
}

void CctControl_ElementDate::OnNextClick(CfxControl *pSender, void *pParams)
{
    _year++;
    Update();
}

void CctControl_ElementDate::OnMonthClick(CfxControl *pSender, void *pParams)
{
    _month = (UINT16)pSender->GetTag();
    Update();
}

void CctControl_ElementDate::OnDayClick(CfxControl *pSender, void *pParams)
{
    _day = (UINT16)pSender->GetTag();
    Update();
}

void CctControl_ElementDate::OnTodayClick(CfxControl *pSender, void *pParams)
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

void CctControl_ElementDate::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawBackArrow(pParams->Rect, _color, _buttonTextColor, _backButton->GetDown());
}

void CctControl_ElementDate::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawNextArrow(pParams->Rect, _color, _buttonTextColor, _nextButton->GetDown());
}

void CctControl_ElementDate::DefineProperties(CfxFiler &F)
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

void CctControl_ElementDate::DefineState(CfxFiler &F)
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

void CctControl_ElementDate::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementDate::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementDate::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    int rowHeight = Height / 10;

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
    
    for (int Month = 0; Month < 12; Month++)
    {
        int Padding = 0;//(Width % 12) / 2;
        double ButtonWidth = (double)Width / 12;
        double Left = ButtonWidth * (double)Month;

        if (Month == 11)
        {
            ButtonWidth = Width - (Padding + Left);
        }

        _monthButtons[Month]->SetBounds(Padding + (int)Left, 0, (int)FxRound(ButtonWidth), rowHeight);
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
    
    for (int Day = 0; Day < 31; Day++)
    {
        int ButtonWidth = Width / 7;
        int Top = (Day / 7) * rowHeight;
        int Left = ButtonWidth * ( Day % 7 );
        int Padding = 0;

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
        bool leapyear = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        days = leapyear ? 29 : 28;
    }

    return days;   
}

void CctControl_ElementDate::Update()
{
    // Get number of day for given year and month
    UINT16 _daysInMonth = GetNumberOfDays(_year, _month);
    for (int i = 0; i < 31; i++)
    {
        _dayButtons[i]->SetVisible(i < _daysInMonth);
    }

    // Select smaller day if _day > _days in month
    if (_day > _daysInMonth)
    {
        _day = _daysInMonth;
    }

    char Buffer[16];
    
    sprintf(Buffer, "%04d-%02d-%02d", _year, _month, _day);
    _panelYearTitle->SetCaption(Buffer);
    sprintf(Buffer, "%04d", _year );
    _panelYear->SetCaption(Buffer);
    _monthButtons[_month-1]->SetDown(true);
    _dayButtons[_day-1]->SetDown(true);

    Repaint();
}
