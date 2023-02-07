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
#include "fxHost.h"
#include "fxUtils.h"

CfxHost::CfxHost(CfxPersistent *pOwner, FXPROFILE *pProfile): CfxPersistent(pOwner)
{  
    _canvas = 0;
    _left = _top = 0;
    _useResourceScale = TRUE;
    _gpsTimeOffset = 0;
    memcpy(&_profile, pProfile, sizeof(FXPROFILE));
}

CfxHost::~CfxHost()
{ 
    StopSound();
    delete _canvas;
};

CfxCanvas *CfxHost::GetCanvas()
{
    if (!_canvas)
    {
        _canvas = CreateCanvas(0, 0);
    }
    return _canvas;
}

VOID CfxHost::GetBounds(FXRECT *pRect)
{ 
    CfxCanvas *canvas = GetCanvas();

    pRect->Left = (INT)_left; 
    pRect->Top = (INT)_top; 
    pRect->Right = (INT)(_left + canvas->GetWidth() - 1);
    pRect->Bottom = (INT)(_top + + canvas->GetHeight() - 1);
}

VOID CfxHost::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    _left = Left;
    _top = Top;
    GetCanvas()->SetSize(Width, Height);
}

BOOL CfxHost::HasExternalCloseButton()
{
    return FALSE;
}

BOOL CfxHost::HasExternalStateData()
{
    return FALSE;
}

VOID CfxHost::HideSIP()
{
}

VOID CfxHost::ShowSIP()
{
}

BOOL CfxHost::IsCameraSupported()
{
    return FALSE;
}

BOOL CfxHost::ShowCameraDialog(CHAR *pFileName, FXIMAGE_QUALITY ImageQuality)
{
    return FALSE;
}

VOID CfxHost::ShowSkyplot(INT /*Left*/, INT /*Top*/, UINT /*Width*/, UINT /*Height*/, BOOL /*Visible*/)
{
}

VOID CfxHost::ShowToast(const CHAR *pMessage)
{
}

VOID CfxHost::ShowExports()
{
}

VOID CfxHost::ShareData()
{
}

BOOL CfxHost::IsBarcodeSupported()
{
    return FALSE;
}

BOOL CfxHost::ShowBarcodeDialog(CHAR *pBarcode)
{
    return FALSE;
}

BOOL CfxHost::IsPhoneNumberSupported()
{
    return FALSE;
}

BOOL CfxHost::ShowPhoneNumberDialog()
{
    return FALSE;
}

BOOL CfxHost::IsEsriSupported()
{
    return FALSE;
}

BOOL CfxHost::TestEsriCredentials(CHAR *pUserName, CHAR *pPassword)
{
    return FALSE;
}

BOOL CfxHost::IsSoundSupported()
{
    return FALSE;
}

BOOL CfxHost::PlaySound(FXSOUNDRESOURCE *pSound)
{
    return FALSE;
}

BOOL CfxHost::StopSound()
{
    return FALSE;
}

BOOL CfxHost::IsPlaying()
{
    return FALSE;
}

BOOL CfxHost::StartRecording(CHAR *pFileNameNoExt, UINT Duration, UINT Frequency)
{
    return FALSE;
}

BOOL CfxHost::StopRecording()
{
    return FALSE;
}

BOOL CfxHost::PlayRecording(CHAR *pFileName)
{
    return FALSE;
}

VOID CfxHost::CancelRecording()
{
}

BOOL CfxHost::IsRecording()
{
    return FALSE;
}

BOOL CfxHost::GetDeviceId(GUID *pGuid)
{
    InitGuid(pGuid);
    return TRUE;
}

CHAR *CfxHost::GetDeviceName()
{
    return AllocString("Not implemented");
}

BOOL CfxHost::CreateGuid(GUID *pGuid)
{
    InitGuid(pGuid);
    return FALSE;
}

VOID CfxHost::GetGPS(FXGPS *pGPS)
{
    memset(pGPS, 0, sizeof(FXGPS));
}

VOID CfxHost::SetGPS(BOOL OnOff)
{
}

VOID CfxHost::ResetGPSTimeouts()
{
}

VOID CfxHost::SetupGPSSimulator(DOUBLE Latitude, DOUBLE Longitude, UINT Radius)
{
}

VOID CfxHost::GetRange(FXRANGE *pRange)
{
    memset(pRange, 0, sizeof(FXRANGE));
}

VOID CfxHost::SetRangeFinder(BOOL OnOff)
{
}

VOID CfxHost::ResetRangeFinderTimeouts()
{
}

BYTE CfxHost::GetGPSPowerupTimeSeconds()
{
    return 1;
}

BOOL CfxHost::HasSmartGPSPower()
{
    return FALSE;
}

VOID CfxHost::SetAlarm(UINT ElapseInSeconds)
{
}

VOID CfxHost::KillAlarm()
{
}

VOID CfxHost::RequestUrl(CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data, CHAR *pFileName, UINT *pId)
{
}

VOID CfxHost::CancelUrl(UINT Id)
{
}

BOOL CfxHost::ConnectPort(BYTE PortId, FX_COMPORT *pComPort)
{
    return FALSE;
}

BOOL CfxHost::IsPortConnected(BYTE PortId)
{
    return FALSE;
}

VOID CfxHost::DisconnectPort(BYTE PortId)
{
}

VOID CfxHost::WritePortData(BYTE PortId, BYTE *pData, UINT Length)
{
}

VOID CfxHost::StartTransfer(CHAR *pOutgoingPath)
{
}

VOID CfxHost::CancelTransfer()
{
}

VOID CfxHost::GetTransferState(FXSENDSTATE *pState)
{
    memset(pState, 0, sizeof(FXSENDSTATE));
}

VOID CfxHost::SetPowerState(FXPOWERMODE Mode)
{
}

FXPOWERMODE CfxHost::GetPowerState()
{
    return powOn;
}

FXBATTERYSTATE CfxHost::GetBatteryState()
{
    return batHigh;
}

UINT8 CfxHost::GetBatteryLevel()
{
    return 100;
}

UINT CfxHost::GetFreeMemory()
{
    return 1000 * 1000;
}

BOOL CfxHost::AppendRecord(CHAR *pFileName, VOID *pRecord, UINT RecordSize)
{
    return FALSE;
}

BOOL CfxHost::DeleteRecords(CHAR *pFileName)
{
    return FALSE;
}

BOOL CfxHost::EnumRecordInit(CHAR *pFileName)
{
    return FALSE;
}

BOOL CfxHost::EnumRecordSetIndex(UINT Index, UINT RecordSize)
{
    return FALSE;
}

BOOL CfxHost::EnumRecordNext(VOID *pRecord, UINT RecordSize)
{
    return FALSE;
}

BOOL CfxHost::EnumRecordDone()
{
    return FALSE;
}

CHAR *CfxHost::AllocPathApplication(const CHAR *pFileName)
{
    return NULL;
}

CHAR *CfxHost::AllocPathSD()
{
    return NULL;
}

BOOL CfxHost::IsUpdateSupported()
{
    return FALSE;
}

VOID CfxHost::CheckForUpdate(GUID AppId, GUID StampId, CHAR *pWebUpdateUrl, CHAR *pTargetPath)
{
}

VOID CfxHost::RegisterExportFile(CHAR *pExportFilePath, FXEXPORTFILEINFO* exportFileInfo)
{

}

VOID CfxHost::ArchiveSighting(GUID* pId, FXDATETIME* dateTime, FXGPS_POSITION* gps, const QVariantMap& content)
{

}

VOID CfxHost::ArchiveWaypoint(GUID* pId, FXDATETIME* dateTime, FXGPS_POSITION* gps)
{

}
