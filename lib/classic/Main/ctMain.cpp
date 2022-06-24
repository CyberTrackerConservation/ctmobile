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

#include "ctMain.h"
#include "ctRegister.h"

CctApplication::CctApplication(CfxHost *pHost): CfxApplication(pHost, new CfxEngine(NULL), new CfxScreen(NULL, NULL))
{
    RegisterCTControls();

    FXRECT rect;
    _host->GetBounds(&rect);
    _screen->SetBounds(rect.Left, rect.Top, rect.Right - rect.Left, rect.Bottom - rect.Top);

    _session = new CctSession(this, _screen);
    _session->SetDock(dkFill);
}

CctApplication::~CctApplication()
{
    delete _session;
}

VOID CctApplication::Run(CHAR *pSequenceName)
{
	_engine->Startup();
	_session->Connect(pSequenceName, 0);
	_session->Run();
}

VOID CctApplication::Startup()
{
    CfxApplication::Startup();
}

VOID CctApplication::Shutdown()
{
    CfxApplication::Shutdown();
}

VOID CctApplication::ShowMenu()
{
    CfxApplication::ShowMenu();
}
