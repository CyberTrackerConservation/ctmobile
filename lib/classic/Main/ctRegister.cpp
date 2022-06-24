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

#include "fxRegister.h"
#include "ctRegister.h"

#include "fxControl.h"
#include "fxDialog.h"
#include "fxAction.h"

#include "ctActions.h"
#include "ctActionControls.h"
#include "ctSession.h"
#include "ctHistory.h"
#include "ctElementDate.h"
#include "ctElementImage.h"
#include "ctElementSound.h"
#include "ctElementList.h"
#include "ctElementLookupList.h"
#include "ctElementKeypad.h"
#include "ctElementTextEdit.h"
#include "ctElementMemo.h"
#include "ctElementCalc.h"
#include "ctElementRecord.h"
#include "ctNavigate.h"
#include "ctNumberList.h"
#include "ctGPSTimerList.h"
#include "ctComPortList.h"
#include "ctElementContainer.h"
#include "ctElementImageGrid.h"
#include "ctElementFilter.h"
#include "ctElementCamera.h"
#include "ctElementPhoneNumber.h"
#include "ctElementBarcode.h"
#include "ctElementR2Incendiary.h"
#include "ctElementSerial.h"
#include "ctElementRangeFinder.h"
#include "ctMovingMap.h"
#include "ctGotos.h"
#include "ctOwnerInfo.h"
#include "ctElementUrlList.h"

#include "ctDialog_SightingEditor.h"
#include "ctDialog_TextEditor.h"
#include "ctDialog_NumberEditor.h"
#include "ctDialog_GPSViewer.h"
#include "ctDialog_GPSReader.h"
#include "ctDialog_Confirm.h"
#include "ctDialog_Password.h"
#include "ctDialog_ComPortSelect.h"

BOOL g_registerCTOnce = FALSE;

VOID RegisterCTControls()
{
    // Ensure once only
    if (g_registerCTOnce) return;
    g_registerCTOnce = TRUE;

    // We requires the standard controls for many of these
    RegisterControls();

    // Invisible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_SESSION,            "Session",              Create_Control_Session,            FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ATTRIBUTEINSPECTOR, "Attribute Inspector",  Create_Control_AttributeInspector, FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_HISTORY_LIST,       "History List",         Create_Control_HistoryList,        FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_MAPITEMLIST,        "Map Item List",        Create_Control_MapItemList,        FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTFILTER,      "Element Filter View",  Create_Control_ElementFilter,      FALSE);

    // Visible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVIGATE,           "Navigator",            Create_Control_Navigate,           TRUE,  ICON(NAVIGATOR),          1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTIMAGE,       "Element Image",        Create_Control_ElementImage,       TRUE,  ICON(ELEMENTIMAGE),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTZOOMIMAGE,   "Element Zoom Image",   Create_Control_ElementZoomImage,   TRUE,  ICON(ELEMENTZOOMIMAGE),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTSOUND,       "Element Sound",        Create_Control_ElementSound,       TRUE,  ICON(ELEMENTSOUND),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTRECORD,      "Element Recorder",     Create_Control_ElementRecord,      TRUE,  ICON(ELEMENTSOUND),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTPANEL,       "Element Panel",        Create_Control_ElementPanel,       TRUE,  ICON(ELEMENTPANEL),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTMEMO,        "Element Memo",         Create_Control_ElementMemo,        TRUE,  ICON(ELEMENTMEMO),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTCONTAINER,   "Element Container",    Create_Control_ElementContainer,   TRUE,  ICON(ELEMENTCONTAINER),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTLIST,        "Element List",         Create_Control_ElementList,        TRUE,  ICON(ELEMENTLIST),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTLOOKUPLIST,  "Element User List",    Create_Control_ElementLookupList,  TRUE,  ICON(ELEMENTLOOKUPLIST),  1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTURLLIST,     "Element Web List",     Create_Control_ElementUrlList,     TRUE,  ICON(ELEMENTLOOKUPLIST),  1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTCALC,        "Element Formula",      Create_Control_ElementCalc,        TRUE,  ICON(ELEMENTCALC),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTKEYPAD,      "Element Keypad",       Create_Control_ElementKeypad,      TRUE,  ICON(ELEMENTKEYPAD),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTEDIT,        "Element Text Edit",    Create_Control_ElementTextEdit,    TRUE,  ICON(ELEMENTEDIT),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTCAMERA,      "Element Camera",       Create_Control_ElementCamera,      TRUE,  ICON(ELEMENTCAMERA),      1);
	ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTBARCODE,     "Element Barcode",      Create_Control_ElementBarcode,     TRUE,  ICON(ELEMENTBARCODE),     1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTPHONENUMBER, "Element Phone number", Create_Control_ElementPhoneNumber, TRUE,  ICON(ELEMENTBARCODE),     1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTRANGEFINDER, "Element RangeFinder",  Create_Control_ElementRangeFinder, TRUE,  ICON(ELEMENTRANGEFINDER), 1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NUMBERLIST,         "Element Number",       Create_Control_NumberList,         TRUE,  ICON(NUMBERLIST),         1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTDATE,        "Element Date",         Create_Control_ElementDate,        TRUE,  ICON(ELEMENTDATE),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_MOVINGMAP,          "Field Map",            Create_Control_MovingMap,          TRUE,  ICON(MOVINGMAP),          1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_MAPINSPECTOR,       "Field Map Inspector",  Create_Control_MapInspector,       TRUE,  ICON(MOVINGMAPINSPECTOR), 1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_GOTOLIST,           "Goto list",            Create_Control_GotoList,           TRUE,  ICON(GOTOLIST),           1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_HISTORY_INSPECTOR,  "History Inspector",    Create_Control_HistoryInspector,   TRUE,  ICON(HISTORYINSPECTOR),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_GPSTIMERLIST,       "GPS Timer List",       Create_Control_GPSTimerList,       TRUE,  ICON(GPSTIMERLIST),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTSERIALDATA,  "Element Serial Data",  Create_Control_ElementSerialData,  TRUE,  ICON(ELEMENTSERIALDATA),  1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_COMPORTLIST,        "Com Port List",        Create_Control_ComPortList,        TRUE,  ICON(COMPORTLIST),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_OWNERINFO,          "Owner Information",    Create_Control_OwnerInfo,          TRUE,  ICON(OWNERINFO),          1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SENDDATA,           "Send Data",            Create_Control_SendData,           TRUE,  ICON(SENDDATA),           1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_BACK,     "Navigator Back",       Create_Control_NavButton_Back,     TRUE,  ICON(NAVBUTTONBACK),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_NEXT,     "Navigator Next",       Create_Control_NavButton_Next,     TRUE,  ICON(NAVBUTTONNEXT),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_HOME,     "Navigator Home",       Create_Control_NavButton_Home,     TRUE,  ICON(NAVBUTTONHOME),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_SKIP,     "Navigator Skip",       Create_Control_NavButton_Skip,     TRUE,  ICON(NAVBUTTONSKIP),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_SAVE,     "Navigator Save",       Create_Control_NavButton_Save,     TRUE,  ICON(NAVBUTTONSAVE),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_OPTIONS,  "Navigator Options",    Create_Control_NavButton_Options,  TRUE,  ICON(NAVBUTTONOPTIONS),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_GPS,      "Navigator GPS",        Create_Control_NavButton_GPS,      TRUE,  ICON(NAVBUTTONGPS),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_JUMP,     "Navigator Jump",       Create_Control_NavButton_Jump,     TRUE,  ICON(NAVBUTTONJUMP),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_ACCEPTEDIT, "Navigator Accept Edit", Create_Control_NavButton_AcceptEdit, FALSE, ICON(NAVBUTTONACCEPTEDIT),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_CANCELEDIT, "Navigator Cancel Edit", Create_Control_NavButton_CancelEdit, FALSE, ICON(NAVBUTTONCANCELEDIT),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_HISTORY,            "Icon Title",           Create_Control_History,            TRUE,  ICON(HISTORY),            1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTIMAGEGRID1,  "Element Image Grid 1", Create_Control_ElementImageGrid1,  TRUE,  ICON(ELEMENTIMAGEGRID),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTIMAGEGRID2,  "Element Image Grid 2", Create_Control_ElementImageGrid2,  TRUE,  ICON(ELEMENTIMAGEGRID),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTR2INCENDIARY, "Element Raindance",   Create_Control_ElementR2Incendiary, TRUE, ICON(ELEMENTR2INCENDIARY), 1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SIGHTINGS,          "Sighting List",        Create_Control_Sightings,          TRUE,  ICON(SIGHTINGLIST),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ATTRIBUTES,         "Attribute List",       Create_Control_Attributes,         TRUE,  ICON(SIGHTINGLIST),       1);

    // Action controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_ADDATTRIBUTE,       "Add Attribute", Create_Control_AddAttribute,             TRUE,  ICON(ACTIONADDATTRIBUTE),         2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ADDUSERNAME,        "Add User Name", Create_Control_AddUserName,              TRUE,  ICON(ACTIONADDUSERNAME),          2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGUREGPS,       "Configure GPS", Create_Control_ConfigureGPS,             TRUE,  ICON(ACTIONCONFIGUREGPS),         2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SNAPGPS,            "Snap GPS Position",        Create_Control_SnapGPS,       TRUE,  ICON(ACTIONSNAPGPS),              2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SNAPLASTGPS,        "Snap Last GPS Position",   Create_Control_SnapLastGPS,   TRUE,  ICON(ACTIONSNAPGPS),              2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SNAPDATETIME,       "Snap Date and Time",       Create_Control_SnapDateTime,  TRUE,  ICON(ACTIONSNAPTIME),             2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGURERANGEFINDER, "Configure RangeFinder",  Create_Control_ConfigureRangeFinder, TRUE,  ICON(ACTIONCONFIGURERANGEFINDER), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGURESAVETARGETS, "Configure Save Targets", Create_Control_ConfigureSaveTargets, TRUE,  ICON(ACTIONCONFIGURESAVETARGETS), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_RESETSTATEKEY,      "Reset state key",          Create_Control_ResetStateKey,  TRUE, ICON(ACTIONRESETSTATEKEY),  2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SETPENDINGGOTO,     "Set pending goto",         Create_Control_SetPendingGoto, TRUE, ICON(ACTIONSETPENDINGGOTO), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_WRITETRACKBREAK,    "Write timer track break",  Create_Control_WriteTrackBreak, TRUE, ICON(ACTIONWRITETRACKBREAK), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGUREALERT,     "Configure alert",          Create_Control_ConfigureAlert, TRUE, ICON(ACTIONWRITETRACKBREAK), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_TRANSFERONSAVE,     "Transfer on save",         Create_Control_TransferOnSave, TRUE, ICON(ACTIONWRITETRACKBREAK), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_REMOVELOOPEDATTRIBUTES, "Remove looped attributes", Create_Control_RemoveLoopedAttributes, TRUE, ICON(ACTIONWRITETRACKBREAK), 2);
    //ControlRegistry.RegisterControl(&GUID_CONTROL_ADDCUSTOMGOTO,      "Add custom goto",          Create_Control_AddCustomGoto,  TRUE,  ICON(ACTIONADDCUSTOMGOTO), 2);

    // Actions
    ActionRegistry.RegisterAction(&GUID_ACTION_APPENDSCREEN,          "Append Screen",        Create_Action_AppendScreen,        TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_INSERTSCREEN,          "Insert Screen",        Create_Action_InsertScreen,        TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_MERGEATTRIBUTE,        "Merge Attribute",      Create_Action_MergeAttribute,      TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGUREGPS,          "Configure GPS",        Create_Action_ConfigureGPS,        TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_SNAPGPS,               "Snap GPS",             Create_Action_SnapGPS,             TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_SNAPDATETIME,          "Snap Date and Time",   Create_Action_SnapDateTime,        TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGURERANGEFINDER,  "Configure RangeFinder", Create_Action_ConfigureRangeFinder, TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGURESAVETARGETS,  "Configure Save Targets", Create_Action_ConfigureSaveTargets, TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_SETGLOBALVALUE,        "Set Global Value",     Create_Action_SetGlobalValue,      TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_SETPENDINGGOTO,        "Set Pending Goto",     Create_Action_SetPendingGoto,      TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_ADDCUSTOMGOTO,         "Add Custom Goto",      Create_Action_AddCustomGoto,       TRUE);
    ActionRegistry.RegisterAction(&GUID_ACTION_SETELEMENTLISTHISTORY, "Set Element List History", Create_Action_SetElementListHistory, FALSE);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGUREALERT,        "Configure Alert",      Create_Action_ConfigureAlert,      FALSE);
    ActionRegistry.RegisterAction(&GUID_ACTION_TRANSFERONSAVE,        "Transfer on Save",     Create_Action_TransferOnSave,      FALSE);

    // Dialogs
    DialogRegistry.RegisterDialog(&GUID_DIALOG_SIGHTINGEDITOR,        Create_Dialog_SightingEditor);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_GPSVIEWER,             Create_Dialog_GPSViewer);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_GPSREADER,             Create_Dialog_GPSReader);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_TEXTEDITOR,            Create_Dialog_TextEditor);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_NUMBEREDITOR,          Create_Dialog_NumberEditor);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_CONFIRM,               Create_Dialog_Confirm);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_PASSWORD,              Create_Dialog_Password);
    DialogRegistry.RegisterDialog(&GUID_DIALOG_PORTSELECT,            Create_Dialog_PortSelect);
}
