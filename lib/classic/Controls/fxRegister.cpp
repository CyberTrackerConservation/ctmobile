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
#include "fxNativeTextEdit.h"
#include "fxSystem.h"

BOOL g_registerOnce = FALSE;

VOID RegisterControls()
{
    // Ensure once only
    if (g_registerOnce) return;
    g_registerOnce = TRUE;

    // Base control
    ControlRegistry.RegisterControl(&GUID_0,                        "Screen",             Create_Control_Screen,           FALSE);

    // Invisible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_BUTTON,           "Button",             Create_Control_Button,           FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SCROLLBARV,       "ScrollBarV",         Create_Control_ScrollBarV,       FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_SCROLLBARV2,      "ScrollBarV2",        Create_Control_ScrollBarV2,      FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_LIST,             "AbstractList",       Create_Control_List,             FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_KEYPAD,           "Keypad",             Create_Control_Keypad,           FALSE);
    ControlRegistry.RegisterControl(&GUID_CONTROL_NATIVETEXTEDIT,   "Native Text Editor", Create_Control_NativeTextEdit,   FALSE);

    // Visible controls
    ControlRegistry.RegisterControl(&GUID_CONTROL_PANEL,            "Panel",              Create_Control_Panel,            TRUE,  ICON(PANEL));
    ControlRegistry.RegisterControl(&GUID_CONTROL_IMAGE,            "Image",              Create_Control_Image,            TRUE,  ICON(IMAGE));
    ControlRegistry.RegisterControl(&GUID_CONTROL_ZOOMIMAGE,        "Zoom Image",         Create_Control_ZoomImage,        TRUE,  ICON(ZOOMIMAGE));
    ControlRegistry.RegisterControl(&GUID_CONTROL_SOUND,            "Sound",              Create_Control_Sound,            TRUE,  ICON(SOUND));
    ControlRegistry.RegisterControl(&GUID_CONTROL_SCROLLBOX,        "Scroll box",         Create_Control_ScrollBox,        TRUE,  ICON(SCROLLBOX));
    ControlRegistry.RegisterControl(&GUID_CONTROL_MARQUEE,          "Marquee",            Create_Control_Marquee,          TRUE,  ICON(MARQUEE));
    ControlRegistry.RegisterControl(&GUID_CONTROL_GPS,              "GPS",                Create_Control_GPS,              TRUE,  ICON(GPS));
    ControlRegistry.RegisterControl(&GUID_CONTROL_RANGEFINDER,      "RangeFinder",        Create_Control_RangeFinder,      TRUE,  ICON(RANGEFINDER));
    ControlRegistry.RegisterControl(&GUID_CONTROL_MEMO,             "Memo",               Create_Control_Memo,             TRUE,  ICON(MEMO));
    ControlRegistry.RegisterControl(&GUID_CONTROL_TITLEBAR,         "Title bar",          Create_Control_TitleBar,         TRUE,  ICON(TITLEBAR));
    ControlRegistry.RegisterControl(&GUID_CONTROL_NOTEBOOK,         "Notebook",           Create_Control_Notebook,         TRUE,  ICON(NOTEBOOK));
    ControlRegistry.RegisterControl(&GUID_CONTROL_SYSTEM,           "System state",       Create_Control_System,           TRUE,  ICON(PANEL));
}
