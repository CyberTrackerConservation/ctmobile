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

#include "ctSightingEditor.h"
#include "ctDialog_NumberEditor.h"

bool g_registerCTOnce = false;

void RegisterCTControls()
{
    // Ensure once only
    if (g_registerCTOnce) return;
    g_registerCTOnce = true;

    // We requires the standard controls for many of these
    RegisterControls();

    // Invisible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_SESSION,            "Session",              Create_Control_Session,            false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ATTRIBUTEINSPECTOR, "Attribute Inspector",  Create_Control_AttributeInspector, false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_HISTORY_LIST,       "History List",         Create_Control_HistoryList,        false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_MAPITEMLIST,        "Map Item List",        Create_Control_MapItemList,        false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTFILTER,      "Element Filter View",  Create_Control_ElementFilter,      false);

    // Visible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVIGATE,           "Navigator",            Create_Control_Navigate,           true,  ICON(NAVIGATOR),          1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTIMAGE,       "Element Image",        Create_Control_ElementImage,       true,  ICON(ELEMENTIMAGE),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTZOOMIMAGE,   "Element Zoom Image",   Create_Control_ElementZoomImage,   true,  ICON(ELEMENTZOOMIMAGE),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTSOUND,       "Element Sound",        Create_Control_ElementSound,       true,  ICON(ELEMENTSOUND),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTRECORD,      "Element Recorder",     Create_Control_ElementRecord,      true,  ICON(ELEMENTSOUND),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTPANEL,       "Element Panel",        Create_Control_ElementPanel,       true,  ICON(ELEMENTPANEL),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTMEMO,        "Element Memo",         Create_Control_ElementMemo,        true,  ICON(ELEMENTMEMO),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTCONTAINER,   "Element Container",    Create_Control_ElementContainer,   true,  ICON(ELEMENTCONTAINER),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTLIST,        "Element List",         Create_Control_ElementList,        true,  ICON(ELEMENTLIST),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTLOOKUPLIST,  "Element User List",    Create_Control_ElementLookupList,  true,  ICON(ELEMENTLOOKUPLIST),  1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTURLLIST,     "Element Web List",     Create_Control_ElementUrlList,     true,  ICON(ELEMENTLOOKUPLIST),  1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTCALC,        "Element Formula",      Create_Control_ElementCalc,        true,  ICON(ELEMENTCALC),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTKEYPAD,      "Element Keypad",       Create_Control_ElementKeypad,      true,  ICON(ELEMENTKEYPAD),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTEDIT,        "Element Text Edit",    Create_Control_ElementTextEdit,    true,  ICON(ELEMENTEDIT),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTCAMERA,      "Element Camera",       Create_Control_ElementCamera,      true,  ICON(ELEMENTCAMERA),      1);
	ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTBARCODE,     "Element Barcode",      Create_Control_ElementBarcode,     true,  ICON(ELEMENTBARCODE),     1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTPHONENUMBER, "Element Phone number", Create_Control_ElementPhoneNumber, true,  ICON(ELEMENTBARCODE),     1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTRANGEFINDER, "Element RangeFinder",  Create_Control_ElementRangeFinder, true,  ICON(ELEMENTRANGEFINDER), 1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NUMBERLIST,         "Element Number",       Create_Control_NumberList,         true,  ICON(NUMBERLIST),         1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTDATE,        "Element Date",         Create_Control_ElementDate,        true,  ICON(ELEMENTDATE),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_MOVINGMAP,          "Field Map",            Create_Control_MovingMap,          true,  ICON(MOVINGMAP),          1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_MAPINSPECTOR,       "Field Map Inspector",  Create_Control_MapInspector,       true,  ICON(MOVINGMAPINSPECTOR), 1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_GOTOLIST,           "Goto list",            Create_Control_GotoList,           true,  ICON(GOTOLIST),           1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_HISTORY_INSPECTOR,  "History Inspector",    Create_Control_HistoryInspector,   true,  ICON(HISTORYINSPECTOR),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_GPSTIMERLIST,       "GPS Timer List",       Create_Control_GPSTimerList,       true,  ICON(GPSTIMERLIST),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTSERIALDATA,  "Element Serial Data",  Create_Control_ElementSerialData,  true,  ICON(ELEMENTSERIALDATA),  1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_COMPORTLIST,        "Com Port List",        Create_Control_ComPortList,        true,  ICON(COMPORTLIST),        1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_OWNERINFO,          "Owner Information",    Create_Control_OwnerInfo,          true,  ICON(OWNERINFO),          1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SENDDATA,           "Send Data",            Create_Control_SendData,           true,  ICON(SENDDATA),           1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SHAREDATA,          "Share Data",           Create_Control_ShareData,          true,  ICON(SHAREDATA),           1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_BACK,     "Navigator Back",       Create_Control_NavButton_Back,     true,  ICON(NAVBUTTONBACK),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_NEXT,     "Navigator Next",       Create_Control_NavButton_Next,     true,  ICON(NAVBUTTONNEXT),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_HOME,     "Navigator Home",       Create_Control_NavButton_Home,     true,  ICON(NAVBUTTONHOME),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_SKIP,     "Navigator Skip",       Create_Control_NavButton_Skip,     true,  ICON(NAVBUTTONSKIP),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_SAVE,     "Navigator Save",       Create_Control_NavButton_Save,     true,  ICON(NAVBUTTONSAVE),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_OPTIONS,  "Navigator Options",    Create_Control_NavButton_Options,  true,  ICON(NAVBUTTONOPTIONS),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_GPS,      "Navigator GPS",        Create_Control_NavButton_GPS,      true,  ICON(NAVBUTTONGPS),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_JUMP,     "Navigator Jump",       Create_Control_NavButton_Jump,     true,  ICON(NAVBUTTONJUMP),      1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_ACCEPTEDIT, "Navigator Accept Edit", Create_Control_NavButton_AcceptEdit, false, ICON(NAVBUTTONACCEPTEDIT),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NAVBUTTON_CANCELEDIT, "Navigator Cancel Edit", Create_Control_NavButton_CancelEdit, false, ICON(NAVBUTTONCANCELEDIT),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_HISTORY,            "Icon Title",           Create_Control_History,            true,  ICON(HISTORY),            1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTIMAGEGRID1,  "Element Image Grid 1", Create_Control_ElementImageGrid1,  true,  ICON(ELEMENTIMAGEGRID),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTIMAGEGRID2,  "Element Image Grid 2", Create_Control_ElementImageGrid2,  true,  ICON(ELEMENTIMAGEGRID),   1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ELEMENTR2INCENDIARY, "Element Raindance",   Create_Control_ElementR2Incendiary, true, ICON(ELEMENTR2INCENDIARY), 1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SIGHTINGS,          "Sighting List",        Create_Control_Sightings,          true,  ICON(SIGHTINGLIST),       1);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ATTRIBUTES,         "Attribute List",       Create_Control_Attributes,         true,  ICON(SIGHTINGLIST),       1);

    // Action controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_ADDATTRIBUTE,       "Add Attribute", Create_Control_AddAttribute,             true,  ICON(ACTIONADDATTRIBUTE),         2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_ADDUSERNAME,        "Add User Name", Create_Control_AddUserName,              true,  ICON(ACTIONADDUSERNAME),          2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGUREGPS,       "Configure GPS", Create_Control_ConfigureGPS,             true,  ICON(ACTIONCONFIGUREGPS),         2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SNAPGPS,            "Snap GPS Position",        Create_Control_SnapGPS,       true,  ICON(ACTIONSNAPGPS),              2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SNAPLASTGPS,        "Snap Last GPS Position",   Create_Control_SnapLastGPS,   true,  ICON(ACTIONSNAPGPS),              2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SNAPDATETIME,       "Snap Date and Time",       Create_Control_SnapDateTime,  true,  ICON(ACTIONSNAPTIME),             2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGURERANGEFINDER, "Configure RangeFinder",  Create_Control_ConfigureRangeFinder, true,  ICON(ACTIONCONFIGURERANGEFINDER), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGURESAVETARGETS, "Configure Save Targets", Create_Control_ConfigureSaveTargets, true,  ICON(ACTIONCONFIGURESAVETARGETS), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_RESETSTATEKEY,      "Reset state key",          Create_Control_ResetStateKey,  true, ICON(ACTIONRESETSTATEKEY),  2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SETPENDINGGOTO,     "Set pending goto",         Create_Control_SetPendingGoto, true, ICON(ACTIONSETPENDINGGOTO), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_WRITETRACKBREAK,    "Write timer track break",  Create_Control_WriteTrackBreak, true, ICON(ACTIONWRITETRACKBREAK), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_CONFIGUREALERT,     "Configure alert",          Create_Control_ConfigureAlert, true, ICON(ACTIONWRITETRACKBREAK), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_TRANSFERONSAVE,     "Transfer on save",         Create_Control_TransferOnSave, true, ICON(ACTIONWRITETRACKBREAK), 2);
    ControlRegistry.RegisterControl(&GUID_CONTROL_REMOVELOOPEDATTRIBUTES, "Remove looped attributes", Create_Control_RemoveLoopedAttributes, true, ICON(ACTIONWRITETRACKBREAK), 2);
    //ControlRegistry.RegisterControl(&GUID_CONTROL_ADDCUSTOMGOTO,      "Add custom goto",          Create_Control_AddCustomGoto,  true,  ICON(ACTIONADDCUSTOMGOTO), 2);

    // Actions
    ActionRegistry.RegisterAction(&GUID_ACTION_APPENDSCREEN,          "Append Screen",        Create_Action_AppendScreen,        true);
    ActionRegistry.RegisterAction(&GUID_ACTION_INSERTSCREEN,          "Insert Screen",        Create_Action_InsertScreen,        true);
    ActionRegistry.RegisterAction(&GUID_ACTION_MERGEATTRIBUTE,        "Merge Attribute",      Create_Action_MergeAttribute,      true);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGUREGPS,          "Configure GPS",        Create_Action_ConfigureGPS,        true);
    ActionRegistry.RegisterAction(&GUID_ACTION_SNAPGPS,               "Snap GPS",             Create_Action_SnapGPS,             true);
    ActionRegistry.RegisterAction(&GUID_ACTION_SNAPDATETIME,          "Snap Date and Time",   Create_Action_SnapDateTime,        true);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGURERANGEFINDER,  "Configure RangeFinder", Create_Action_ConfigureRangeFinder, true);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGURESAVETARGETS,  "Configure Save Targets", Create_Action_ConfigureSaveTargets, true);
    ActionRegistry.RegisterAction(&GUID_ACTION_SETGLOBALVALUE,        "Set Global Value",     Create_Action_SetGlobalValue,      true);
    ActionRegistry.RegisterAction(&GUID_ACTION_SETPENDINGGOTO,        "Set Pending Goto",     Create_Action_SetPendingGoto,      true);
    ActionRegistry.RegisterAction(&GUID_ACTION_ADDCUSTOMGOTO,         "Add Custom Goto",      Create_Action_AddCustomGoto,       true);
    ActionRegistry.RegisterAction(&GUID_ACTION_SETELEMENTLISTHISTORY, "Set Element List History", Create_Action_SetElementListHistory, false);
    ActionRegistry.RegisterAction(&GUID_ACTION_CONFIGUREALERT,        "Configure Alert",      Create_Action_ConfigureAlert,      false);
    ActionRegistry.RegisterAction(&GUID_ACTION_TRANSFERONSAVE,        "Transfer on Save",     Create_Action_TransferOnSave,      false);

    // Dialogs
    DialogRegistry.RegisterDialog(&GUID_DIALOG_NUMBEREDITOR,          Create_Dialog_NumberEditor);
}
