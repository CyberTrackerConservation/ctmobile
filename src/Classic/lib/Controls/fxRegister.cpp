#include "fxRegister.h"

#include "fxTypes.h"
#include "fxClasses.h"
#include "fxControl.h"
#include "fxDialog.h"
#include "fxAction.h"
#include "fxPanel.h"
#include "fxButton.h"
#include "fxScrollBar.h"
#include "fxImage.h"
#include "fxSound.h"
#include "fxList.h"
#include "fxScrollBox.h"
#include "fxKeypad.h"
#include "fxMarquee.h"
#include "fxGPS.h"
#include "fxRangeFinder.h"
#include "fxMemo.h"
#include "fxTitleBar.h"
#include "fxNotebook.h"
#include "fxSystem.h"

bool g_registerOnce = false;

void RegisterControls()
{
    // Ensure once only
    if (g_registerOnce) return;
    g_registerOnce = true;

    // Base control
    ControlRegistry.RegisterControl(&GUID_0,                        "Screen",             Create_Control_Screen,           false);

    // Invisible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_BUTTON,           "Button",             Create_Control_Button,           false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SCROLLBARV,       "ScrollBarV",         Create_Control_ScrollBarV,       false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SCROLLBARV2,      "ScrollBarV2",        Create_Control_ScrollBarV2,      false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_LIST,             "AbstractList",       Create_Control_List,             false);
    ControlRegistry.RegisterControl(&GUID_CONTROL_KEYPAD,           "Keypad",             Create_Control_Keypad,           false);

    // Visible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_PANEL,            "Panel",              Create_Control_Panel,            true,  ICON(PANEL));
    ControlRegistry.RegisterControl(&GUID_CONTROL_IMAGE,            "Image",              Create_Control_Image,            true,  ICON(IMAGE));
    ControlRegistry.RegisterControl(&GUID_CONTROL_ZOOMIMAGE,        "Zoom Image",         Create_Control_ZoomImage,        true,  ICON(ZOOMIMAGE));
    ControlRegistry.RegisterControl(&GUID_CONTROL_SOUND,            "Sound",              Create_Control_Sound,            true,  ICON(SOUND));
    ControlRegistry.RegisterControl(&GUID_CONTROL_SCROLLBOX,        "Scroll box",         Create_Control_ScrollBox,        true,  ICON(SCROLLBOX));
    ControlRegistry.RegisterControl(&GUID_CONTROL_MARQUEE,          "Marquee",            Create_Control_Marquee,          true,  ICON(MARQUEE));
    ControlRegistry.RegisterControl(&GUID_CONTROL_GPS,              "GPS",                Create_Control_GPS,              true,  ICON(GPS));
    ControlRegistry.RegisterControl(&GUID_CONTROL_RANGEFINDER,      "RangeFinder",        Create_Control_RangeFinder,      true,  ICON(RANGEFINDER));
    ControlRegistry.RegisterControl(&GUID_CONTROL_MEMO,             "Memo",               Create_Control_Memo,             true,  ICON(MEMO));
    ControlRegistry.RegisterControl(&GUID_CONTROL_TITLEBAR,         "Title bar",          Create_Control_TitleBar,         true,  ICON(TITLEBAR));
    ControlRegistry.RegisterControl(&GUID_CONTROL_NOTEBOOK,         "Notebook",           Create_Control_Notebook,         true,  ICON(NOTEBOOK));
    ControlRegistry.RegisterControl(&GUID_CONTROL_SYSTEM,           "System state",       Create_Control_System,           true,  ICON(PANEL));
}
