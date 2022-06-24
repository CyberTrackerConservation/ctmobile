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

#include "fxApplication.h"
#include "fxControl.h"
#include "ctSession.h"
#include "ctDialog_GPSViewer.h"

//*************************************************************************************************
// CctDialog_GPSViewer

CfxDialog *Create_Dialog_GPSViewer(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_GPSViewer(pOwner, pParent);
}

CctDialog_GPSViewer::CctDialog_GPSViewer(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_GPSVIEWER);

    _dock = dkFill;

    _originalId = 0;

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("GPS");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(FALSE);
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetShowBattery(TRUE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 24, 24);

    _spacer = new CfxControl_Panel(pOwner, this);
    _spacer->SetComposite(TRUE);
    _spacer->SetBorderStyle(bsNone);
    _spacer->SetDock(dkTop);
    _spacer->SetBounds(0, 1, 20, 4);

    _noteBook = new CfxControl_Notebook(pOwner, this);
    _noteBook->SetComposite(TRUE);
    _noteBook->SetDock(dkFill);
    _noteBook->SetCaption("Fix;Sky;Compass;Waypoint;Map");
    _noteBook->ProcessPages();
    _noteBook->SetOnPageChange(this, (NotifyMethod)&CctDialog_GPSViewer::OnNoteBookPageChange);

    CfxControl_GPS *gps;
    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(0));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkTop);
    gps->SetBounds(0, 0, 100, 32);
    gps->SetStyle(gcsState);    
    gps->SetAutoHeight(TRUE);
    gps->SetFont(F_FONT_DEFAULT_LB);

    _travelMode = new CfxControl_Panel(pOwner, this);
    _travelMode->SetBorderStyle(bsNone);
    _travelMode->SetDock(dkTop);
    _travelMode->SetBounds(0, 0, 100, 32);
    _travelMode->SetAutoHeight(TRUE);
    _travelMode->SetFont(F_FONT_DEFAULT_MB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(0));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsData);
    gps->SetExtraData(TRUE);
    gps->SetFont(F_FONT_DEFAULT_LB);
    
    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(1));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkTop);
    gps->SetBounds(0, 0, 100, 32);
    gps->SetStyle(gcsState);    
    gps->SetAutoHeight(TRUE);
    gps->SetFont(F_FONT_DEFAULT_LB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(1));
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(4);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsSky);
    gps->SetFont(F_FONT_FIXED_S);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(1));
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(4);
    gps->SetDock(dkBottom);
    gps->SetBounds(0, 0, 100, 64);
    gps->SetStyle(gcsBars);
    gps->SetFont(F_FONT_FIXED_S);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(2));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkTop);
    gps->SetBounds(0, 0, 100, 32);
    gps->SetStyle(gcsState);
    gps->SetAutoHeight(TRUE);
    gps->SetFont(F_FONT_DEFAULT_LB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(2));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkBottom);
    gps->SetBounds(0, 0, 100, 20);
    gps->SetStyle(gcsHeading);
    gps->SetBorderWidth(2);
    gps->SetAutoHeight(TRUE);
    gps->SetFont(F_FONT_DEFAULT_M);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(2));
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(4);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsCompass2);
    gps->SetFont(F_FONT_DEFAULT_M);

    _movingMap = new CctControl_MovingMap(pOwner, this);
    _movingMap->SetComposite(TRUE);
    _movingMap->SetBorderStyle(bsNone);
    _movingMap->SetBorderWidth(0);
    _movingMap->SetDock(dkFill);
    _movingMap->SetPositionFlag(FALSE);
    _movingMap->SetShowInspect(TRUE);

    _mapInspector = new CctControl_MapInspector(pOwner, this);
    _mapInspector->SetComposite(TRUE);
    _mapInspector->SetBorderStyle(bsNone);
    _mapInspector->SetBorderWidth(0);
    _mapInspector->SetDock(dkFill);

    _gotoList = new CctControl_GotoList(pOwner, this);
    _gotoList->SetComposite(TRUE);
    _gotoList->SetBorderStyle(bsNone);
    _gotoList->SetBorderWidth(0);
    _gotoList->SetDock(dkFill);
    _gotoList->SetOnClick(this, (NotifyMethod)&CctDialog_GPSViewer::OnNewGotoClick);
    _gotoList->SetFont(F_FONT_DEFAULT_LB);
    
    _gotoData = new CfxControl_Panel(pOwner, this);
    _gotoData->SetComposite(TRUE);
    _gotoData->SetBorderStyle(bsNone);
    _gotoData->SetBorderWidth(0);
    _gotoData->SetDock(dkFill);

    gps = new CfxControl_GPS(pOwner, _gotoData);
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(1);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsGotoPointer);
    gps->SetFont(F_FONT_DEFAULT_XLB);
    gps->SetBounds(0, 0, 100, 80);

    gps = new CfxControl_GPS(pOwner, _gotoData);
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(1);
    gps->SetDock(dkBottom);
    gps->SetStyle(gcsGotoData);
    gps->SetFont(F_FONT_DEFAULT_XLB);
    gps->SetBounds(0, 100, 100, 100);
    gps->SetAutoHeight(TRUE);

	CfxControl_Button *button;
	button = new CfxControl_Button(pOwner, _gotoData);
    button->SetFont(F_FONT_DEFAULT_LB);
	button->SetCaption("Change");
	button->SetDock(dkBottom);
	button->SetBounds(0, 1000, 24, 32);
	button->SetOnClick(this, (NotifyMethod)&CctDialog_GPSViewer::OnSetGotoClick);

    if (IsLive())
    {
        GetEngine(this)->LockGPS();
    }
}

CctDialog_GPSViewer::~CctDialog_GPSViewer()
{
    if (IsLive())
    {
        GetHost(this)->ShowSkyplot(0, 0, 0, 0, false);
        GetEngine(this)->UnlockGPS();
    }
    _titleBar = NULL;
    _spacer = NULL;
    _noteBook = NULL;
    _movingMap = NULL;
    _mapInspector = NULL;
    _gotoList = NULL;
    _gotoData = NULL;
    _travelMode = NULL;
}

VOID CctDialog_GPSViewer::OnNoteBookPageChange(CfxControl * pSender, VOID *pParams)
{
    auto skyPage = _noteBook->GetPage(1);
    skyPage->ClearControls();

    FXRECT r;
    skyPage->GetAbsoluteBounds(&r);

    GetHost(this)->ShowSkyplot(r.Left, r.Top, r.Right - r.Left + 1, r.Bottom - r.Top + 1, _noteBook->GetActivePageIndex() == 2);
}

VOID CctDialog_GPSViewer::OnSetGotoClick(CfxControl *pSender, VOID *pParams)
{
    _gotoList->SetVisible(TRUE);
    _gotoData->SetVisible(FALSE);
    GetEngine(this)->AlignControls(_gotoList->GetParent());
    Repaint();
}

VOID CctDialog_GPSViewer::OnNewGotoClick(CfxControl *pSender, VOID *pParams)
{
    CfxEngine *engine = GetEngine(this);

    FXGOTO *pgoto = engine->GetCurrentGoto();

    if (pgoto && pgoto->Title[0])
    {
        _gotoList->SetVisible(FALSE);
        _gotoData->SetVisible(TRUE);

		CfxControl *button = _gotoData->FindControlByType(&GUID_CONTROL_BUTTON);
		button->SetBounds(0, _noteBook->GetHeight() + 100, button->GetWidth(), button->GetHeight());
		engine->AlignControls(_gotoData->GetParent());
        Repaint();
    }
}

VOID CctDialog_GPSViewer::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    CctSession *session = GetSession(this);
    _originalId = session->GetCurrentSightingId(); 

    // Reload last state
    STOREITEM *item = session->GetStoreItem(GUID_DIALOG_GPSVIEWER, 0);
    if (item)
    {
        CfxStream stream(item->Data);
        CfxReader reader(&stream);
        DefineState(reader);
    }

    CHAR pages[256] = "Fix;Sky;Compass";
    UINT nextPageIndex = 3;

    //
    // Setup the Goto tab
    //
    BOOL gotoTab = IsLive();// &&
                   //((session->GetResourceHeader()->GotosCount > 0) ||
                   // (session->GetCustomGotoCount() > 0));
    UINT gotoPageIndex;
    if (gotoTab)
    {
        strcat(pages, ";Goto");
        gotoPageIndex = nextPageIndex;
        nextPageIndex++;
    }
    _gotoList->SetVisible(FALSE);
    _gotoData->SetVisible(FALSE);

    //
    // Setup the Map tab
    //
    BOOL movingMapTab = FALSE;

    movingMapTab = IsLive() && GetResource() && 
                   (GetCanvas(this)->GetBitDepth() > 1) &&
                   (session->GetResourceHeader()->MovingMapsCount > 0);

    UINT mapPageIndex = 0, mapInspectorPageIndex = 0;
    if (movingMapTab)
    {
        strcat(pages, ";Map");
        mapPageIndex = nextPageIndex;
        nextPageIndex++;
        strcat(pages, ";Inspect");
        mapInspectorPageIndex = nextPageIndex;
        nextPageIndex++;
    }
    _movingMap->SetVisible(FALSE);
    _mapInspector->SetVisible(FALSE);

    //
    // Set up the notebook
    //
    _noteBook->SetCaption(pages);
    _noteBook->ProcessPages();

    //
    // Finalize travel mode
    //
    _travelMode->SetParent(_noteBook->GetPage(0));
    if (session->GetSightingAccuracyName())
    {
        _travelMode->SetVisible(TRUE);
        _travelMode->SetCaption(session->GetSightingAccuracyName());
    }
    else
    {
        _travelMode->SetVisible(FALSE);
    }
    PropagateGPSAccuracy(_noteBook->GetPage(0), session->GetSightingAccuracy());

    //
    // Finalize goto
    //
    if (gotoTab)
    {
        _gotoList->SetParent(_noteBook->GetPage(gotoPageIndex));
        _gotoData->SetParent(_noteBook->GetPage(gotoPageIndex));

        FXGOTO *pgoto = GetEngine(this)->GetCurrentGoto();
        if (pgoto && pgoto->Title[0])
        {
            _gotoList->SetVisible(FALSE);
            _gotoData->SetVisible(TRUE);
        }
        else
        {
            _gotoList->SetVisible(TRUE);
            _gotoData->SetVisible(FALSE);
        }
    }

    //
    // Finalize map.
    //
    if (movingMapTab)
    {
        _movingMap->SetParent(_noteBook->GetPage(mapPageIndex));
        _movingMap->SetVisible(TRUE);
        
		_movingMap->LoadGlobalState();
        
        _mapInspector->SetParent(_noteBook->GetPage(mapInspectorPageIndex));
        _mapInspector->SetVisible(TRUE);
    }
}

VOID CctDialog_GPSViewer::PostInit(CfxControl *pSender)
{
    OnNoteBookPageChange(this, NULL);
}

VOID CctDialog_GPSViewer::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;

    CctSession *session = GetSession(this);

    // Remove travel mode
    _travelMode->SetVisible(FALSE);
    _travelMode->SetParent(this);

    // Remove the map from the Notebook and save its state
    _movingMap->SetVisible(FALSE);
    _movingMap->SetParent(this);
    _movingMap->SaveGlobalState();
    _mapInspector->SetVisible(FALSE);
    _mapInspector->SetParent(this);

    // Remove the goto state from the Notebook
    _gotoList->SetVisible(FALSE);
    _gotoList->SetParent(this);
    _gotoData->SetVisible(FALSE);
    _gotoData->SetParent(this);

    // Store last dialog state
    CfxStream stream;
    CfxWriter writer(&stream);
    DefineState(writer);
    GetSession(this)->SetStoreItem(GUID_DIALOG_GPSVIEWER, 0, stream.CloneMemory());

    // Shutdown and reconnect 
    ClearControls();
    GetSession(this)->Reconnect(_originalId);
}

VOID CctDialog_GPSViewer::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Spacer"))
    {
        _spacer->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Notebook"))
    {
        _noteBook->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("MovingMap"))
    {
        _movingMap->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("MapInspector"))
    {
        _mapInspector->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Goto"))
    {
        _gotoList->DefineProperties(F);
        _gotoData->DefineProperties(F);
        F.DefineEnd();
    }
}

VOID CctDialog_GPSViewer::DefineState(CfxFiler &F)
{
    _noteBook->DefineState(F);
    _gotoList->DefineState(F);
}

VOID CctDialog_GPSViewer::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _noteBook->DefineResources(F);
    _movingMap->DefineResources(F);
    _mapInspector->DefineResources(F);
    _gotoList->DefineResources(F);
    _gotoData->DefineResources(F);
    _travelMode->DefineResources(F);
#endif
}
