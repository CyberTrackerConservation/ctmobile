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
#include "fxUtils.h"
#include "Win32_WAVE.h"
#include <mmsystem.h>

struct WAVE_RECORDING
{
    BYTE *Buffer;
    UINT BufferSize;

    HWND hWnd;

    HWAVEIN hwi;
    WAVEHDR hdr;
    WAVEFORMATEX wfx;
};

WAVE_RECORDING *g_WaveRecording;

#pragma pack(push, 1) 
typedef struct
{
   DWORD   dwRiff;     // Type of file header.
   DWORD   dwSize;     // Size of file header.
   DWORD   dwWave;     // Type of wave.
} RIFF_FILEHEADER, *PRIFF_FILEHEADER;

typedef struct
{
   DWORD   dwCKID;        // Type Identification for current chunk header.
   DWORD   dwSize;        // Size of current chunk header.
} RIFF_CHUNKHEADER, *PRIFF_CHUNKHEADER;

// Chunk Types  
#define RIFF_FILE       mmioFOURCC('R','I','F','F')
#define RIFF_WAVE       mmioFOURCC('W','A','V','E')
#define RIFF_FORMAT     mmioFOURCC('f','m','t',' ')
#define RIFF_CHANNEL    mmioFOURCC('d','a','t','a')
#pragma pack(pop) 

MMRESULT WriteWaveFile(PCWSTR pszFilename, PWAVEFORMATEX pWFX, DWORD dwBufferSize, PBYTE pBufferBits)
{ 
    RIFF_FILEHEADER FileHeader;
    RIFF_CHUNKHEADER WaveHeader;
    RIFF_CHUNKHEADER DataHeader;
    DWORD dwBytesWritten;
    HANDLE fh;
    MMRESULT mmRet = MMSYSERR_ERROR;

    // Fill in the file, wave and data headers
    WaveHeader.dwCKID = RIFF_FORMAT;
    WaveHeader.dwSize = sizeof(WAVEFORMATEX) + pWFX->cbSize;

    // the DataHeader chunk contains the audio data
    DataHeader.dwCKID = RIFF_CHANNEL;
    DataHeader.dwSize = dwBufferSize;

    // The FileHeader
    FileHeader.dwRiff = RIFF_FILE;
    FileHeader.dwWave = RIFF_WAVE;
    FileHeader.dwSize = sizeof(FileHeader.dwWave)+sizeof(WaveHeader) + WaveHeader.dwSize + sizeof(DataHeader) + DataHeader.dwSize;

    // Open wave file
    fh = CreateFileW(pszFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (fh == INVALID_HANDLE_VALUE)
    {
        return mmRet;
    }

    // Write the riff file
    if (!WriteFile(fh, &FileHeader, sizeof(FileHeader), &dwBytesWritten, NULL)) 
    {
        goto ERROR_EXIT;
    }

    // Write the wave header
    if (!WriteFile(fh, &WaveHeader, sizeof(WaveHeader), &dwBytesWritten, NULL)) 
    {
        goto ERROR_EXIT;
    }

    // Write the wave format
    if (!WriteFile(fh, pWFX, WaveHeader.dwSize, &dwBytesWritten, NULL)) 
    {
        goto ERROR_EXIT;
    }

    // Write the data header
    if (!WriteFile(fh, &DataHeader, sizeof(DataHeader), &dwBytesWritten, NULL)) 
    {
        goto ERROR_EXIT;
    }

    // Write the PCM data
    if (!WriteFile(fh, pBufferBits, DataHeader.dwSize, &dwBytesWritten, NULL)) 
    {
        goto ERROR_EXIT;
    }

    // Success
    mmRet = MMSYSERR_NOERROR;

  ERROR_EXIT:

    CloseHandle(fh);
    return mmRet;
}

VOID *WAVE_StartRecording(HWND hWnd, UINT Duration, UINT Frequency)
{
    WAVE_FreeRecording(g_WaveRecording);

    WAVE_RECORDING *wr = (WAVE_RECORDING *)MEM_ALLOC(sizeof(WAVE_RECORDING));
    BOOL success = FALSE;
    MMRESULT mr;

    memset(wr, 0, sizeof(WAVE_RECORDING));

    // Set up basic data structure.
    wr->hWnd = hWnd;
    wr->wfx.cbSize = 0;
    wr->wfx.wFormatTag = WAVE_FORMAT_PCM;
    wr->wfx.wBitsPerSample = 16;
    wr->wfx.nSamplesPerSec = Frequency;
    wr->wfx.nChannels = 1;
    wr->wfx.nBlockAlign = wr->wfx.nChannels * wr->wfx.wBitsPerSample / 8;
    wr->wfx.nAvgBytesPerSec = wr->wfx.nBlockAlign * wr->wfx.nSamplesPerSec;
    wr->BufferSize = (DWORD)((((UINT64)Duration) * ((UINT64)wr->wfx.nAvgBytesPerSec) / 1000));
    wr->BufferSize += wr->BufferSize % wr->wfx.nBlockAlign;
    wr->Buffer = (BYTE *)MEM_ALLOC(wr->BufferSize);
    if (wr->Buffer == NULL)
    {
		Log("Memory allocation failed %d", GetLastError());
        goto cleanup;
    }
    wr->hdr.dwBufferLength = wr->BufferSize;
    wr->hdr.lpData = (CHAR *) wr->Buffer;

    // open the wave capture device
    mr = waveInOpen(&wr->hwi, WAVE_MAPPER, &wr->wfx, (DWORD)hWnd, NULL, CALLBACK_WINDOW);
    if (mr == WAVERR_BADFORMAT)
    {
        // Try again with different parameters
        MEM_FREE(wr->Buffer);
        wr->Buffer = NULL;
        wr->hdr.lpData = NULL;

        wr->wfx.wFormatTag = WAVE_FORMAT_PCM;
        wr->wfx.nChannels = 1;
        wr->wfx.wBitsPerSample = 8;
        wr->wfx.cbSize = 0;
        wr->wfx.nSamplesPerSec = 8000;
        wr->wfx.nBlockAlign = wr->wfx.nChannels * wr->wfx.wBitsPerSample / 8;
        wr->wfx.nAvgBytesPerSec = wr->wfx.nBlockAlign * wr->wfx.nSamplesPerSec;
        wr->BufferSize = (DWORD)((((UINT64)Duration) * ((UINT64)wr->wfx.nAvgBytesPerSec) / 1000));
        wr->BufferSize += wr->BufferSize % wr->wfx.nBlockAlign;
        wr->Buffer = (BYTE *)MEM_ALLOC(wr->BufferSize);
        if (wr->Buffer == NULL)
        {
		    Log("Memory allocation failed %d", GetLastError());
            goto cleanup;
        }
        wr->hdr.dwBufferLength = wr->BufferSize;
        wr->hdr.lpData = (CHAR *) wr->Buffer;
        mr = waveInOpen(&wr->hwi, WAVE_MAPPER, &wr->wfx, (DWORD)hWnd, NULL, CALLBACK_WINDOW);
    }

    if (mr != MMSYSERR_NOERROR) 
    {
		Log("waveInOpen failed %08lx", mr);
        goto cleanup;
    }

    // set up the WAVEHDR structure that describes the capture buffer
    // prepare the buffer for capture
    mr = waveInPrepareHeader(wr->hwi, &wr->hdr, sizeof(WAVEHDR));
    if (mr != MMSYSERR_NOERROR) 
    {
		Log("waveInPrepareHeader failed %08lx", mr);
        goto cleanup;
    }

    // submit the buffer for capture
    mr = waveInAddBuffer(wr->hwi, &wr->hdr, sizeof(WAVEHDR));
    if (mr != MMSYSERR_NOERROR) 
    {
		Log("waveInAddBuffer failed %08lx", mr);
        goto cleanup;
    }

    // start capturing
    mr = waveInStart(wr->hwi);
    if (mr != MMSYSERR_NOERROR) 
    {
		Log("waveInStart failed %08lx", mr);
        goto cleanup;
    }

    g_WaveRecording = wr;
    wr = NULL;
    success = TRUE;

  cleanup:

    if (wr)
    {
        // close the capture device & MEM_FREE the event handle
        if (wr->hwi != 0)
        {
            mr = waveInClose(wr->hwi);
            if (mr != MMSYSERR_NOERROR) {
				Log("waveInClose failed %08lx", mr);
            }
            wr->hwi = 0;
        }

        if (wr->Buffer != NULL)
        {
            MEM_FREE(wr->Buffer);
            wr->Buffer = NULL;
        }
    }
    
    if (success)
    {
        return g_WaveRecording;
    }
    else
    {
        return NULL;
    }
}

VOID WAVE_FreeRecording(VOID *pWaveRecording)
{
    MMRESULT mr;
    WAVE_RECORDING *p = (WAVE_RECORDING *)pWaveRecording;

    if (p == NULL) return;

    if (p->hwi != 0)
    {
        mr = waveInReset(p->hwi);
        //mr = waveInStop(p->hwi);

        mr = waveInClose(p->hwi);
        if (mr != MMSYSERR_NOERROR) 
        {
        }
    }

    if (p->Buffer != NULL)
    {
        MEM_FREE(p->Buffer);
        p->Buffer = NULL;
    }

    memset(p, 0, sizeof(WAVE_RECORDING));
}

BOOL WAVE_StopRecording(VOID *pWaveRecording)
{
    MMRESULT mr;
    WAVE_RECORDING *p = (WAVE_RECORDING *)pWaveRecording;
    if (p == NULL) return FALSE;

    mr = waveInReset(p->hwi);
    if (mr != MMSYSERR_NOERROR) 
    {
    }

    // now clean up: unprepare the buffer
    mr = waveInUnprepareHeader(p->hwi, &p->hdr, sizeof(WAVEHDR));
    if (mr != MMSYSERR_NOERROR) 
    {
    }

    mr = waveInClose(p->hwi);
    if (mr != MMSYSERR_NOERROR) 
    {
    }

    p->hwi = 0;

    return TRUE;
}

BOOL WAVE_SaveRecording(VOID *pWaveRecording, WCHAR *pFileName, UINT *pDuration)
{
    MMRESULT mr;
    WAVE_RECORDING *p = (WAVE_RECORDING *)pWaveRecording;
    if ((p == NULL) || (p->hdr.dwBytesRecorded == 0)) 
	{
        return FALSE;
    }

    mr = WriteWaveFile(pFileName, &p->wfx, p->hdr.dwBytesRecorded, p->Buffer);

    if (pDuration != NULL) 
	{
        *pDuration = (p->hdr.dwBytesRecorded * 1000) / p->wfx.nAvgBytesPerSec;
    }

    return mr == MMSYSERR_NOERROR;
}

