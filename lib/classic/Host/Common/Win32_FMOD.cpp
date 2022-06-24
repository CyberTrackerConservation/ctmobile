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

#include "Win32_FMOD.h"
#include "Win32_Utils.h"
#include <tchar.h>

#ifdef _WIN32_WCE
    #define FSOUND_DLL                L"fmodce"
    #define API_FSOUND_Init           "FSOUND_Init"
    #define API_FSOUND_Close          "FSOUND_Close"
    #define API_FSOUND_GetError       "FSOUND_GetError"
    #define API_FSOUND_Sample_Load    "FSOUND_Sample_Load"
    #define API_FSOUND_Sample_Free    "FSOUND_Sample_Free"
    #define API_FSOUND_PlaySound      "FSOUND_PlaySound"
    #define API_FSOUND_IsPlaying      "FSOUND_IsPlaying"
    #define API_FSOUND_GetMaxChannels "FSOUND_GetMaxChannels"

#else
    #define FSOUND_DLL                L"fmod"
    #define API_FSOUND_Init           "_FSOUND_Init@12"
    #define API_FSOUND_Close          "_FSOUND_Close@0"
    #define API_FSOUND_GetError       "_FSOUND_GetError@0"
    #define API_FSOUND_Sample_Load    "_FSOUND_Sample_Load@20"
    #define API_FSOUND_Sample_Free    "_FSOUND_Sample_Free@4"
    #define API_FSOUND_PlaySound      "_FSOUND_PlaySound@8"
    #define API_FSOUND_IsPlaying      "_FSOUND_IsPlaying@4"
    #define API_FSOUND_GetMaxChannels "_FSOUND_GetMaxChannels@0"
#endif

typedef BOOL  (WINAPI *FN_FSOUND_Init)(INT mixrate, INT maxsoftwarechannels, UINT flags);
typedef BOOL  (WINAPI *FN_FSOUND_Close)();
typedef INT (WINAPI *FN_FSOUND_GetError)();
typedef VOID* (WINAPI *FN_FSOUND_Sample_Load)(INT index, CHAR *name_or_data, UINT mode, INT offset, INT length);
typedef VOID  (WINAPI *FN_FSOUND_Sample_Free)(VOID *sptr);
typedef INT (WINAPI *FN_FSOUND_PlaySound)(INT channel, VOID *sptr);
typedef BOOL  (WINAPI *FN_FSOUND_IsPlaying)(INT channel);
typedef INT (WINAPI *FN_FSOUND_GetMaxChannels)();

BOOL g_bSoundInit = FALSE;
BOOL g_bSoundValid = FALSE;

FN_FSOUND_Init g_fn_FSOUND_Init;
FN_FSOUND_Close g_fn_FSOUND_Close;
FN_FSOUND_GetError g_fn_FSOUND_GetError;
FN_FSOUND_Sample_Load g_fn_FSOUND_Sample_Load;
FN_FSOUND_Sample_Free g_fn_FSOUND_Sample_Free;
FN_FSOUND_PlaySound g_fn_FSOUND_PlaySound;
FN_FSOUND_IsPlaying g_fn_FSOUND_IsPlaying;
FN_FSOUND_GetMaxChannels g_fn_FSOUND_GetMaxChannels;

BOOL InitSoundLibrary()
{
    if (g_bSoundInit) return g_bSoundValid;

    g_bSoundInit = TRUE;
    g_bSoundValid = FALSE;

    HMODULE hLib = LoadLibraryW(FSOUND_DLL);

    if (hLib)
    {
        g_fn_FSOUND_Init           = (FN_FSOUND_Init)GetProcAddress(hLib,  _T(API_FSOUND_Init));
        g_fn_FSOUND_Close          = (FN_FSOUND_Close)GetProcAddress(hLib, _T(API_FSOUND_Close));
        g_fn_FSOUND_GetError       = (FN_FSOUND_GetError)GetProcAddress(hLib, _T(API_FSOUND_GetError));
        g_fn_FSOUND_Sample_Load    = (FN_FSOUND_Sample_Load)GetProcAddress(hLib, _T(API_FSOUND_Sample_Load));
        g_fn_FSOUND_Sample_Free    = (FN_FSOUND_Sample_Free)GetProcAddress(hLib, _T(API_FSOUND_Sample_Free));
        g_fn_FSOUND_PlaySound      = (FN_FSOUND_PlaySound)GetProcAddress(hLib, _T(API_FSOUND_PlaySound));
        g_fn_FSOUND_IsPlaying      = (FN_FSOUND_IsPlaying)GetProcAddress(hLib, _T(API_FSOUND_IsPlaying));
        g_fn_FSOUND_GetMaxChannels = (FN_FSOUND_GetMaxChannels)GetProcAddress(hLib, _T(API_FSOUND_GetMaxChannels));
    }

    BOOL apiOkay = g_fn_FSOUND_Init &&
                   g_fn_FSOUND_Close &&
                   g_fn_FSOUND_GetError &&
                   g_fn_FSOUND_Sample_Load &&
                   g_fn_FSOUND_Sample_Free &&
                   g_fn_FSOUND_PlaySound &&
                   g_fn_FSOUND_IsPlaying &&
                   g_fn_FSOUND_GetMaxChannels;

    if (apiOkay)
    {
        g_bSoundValid = g_fn_FSOUND_Init(44100, 32, 1);
    }

    return g_bSoundValid;
}

#define FSOUND_FREE          -1          // value to play on any MEM_FREE channel, or to allocate a sample in a MEM_FREE sample slot. 
#define FSOUND_LOADMEMORY    0x00008000  // "name" will be interpreted as a pointer to data for streaming and samples. 
#define FSOUND_LOOP_OFF      0x00000001  // For non looping samples

VOID *FMOD_Play(VOID *pData, UINT Size)
{
    if (!InitSoundLibrary()) return NULL;

    VOID *sample = g_fn_FSOUND_Sample_Load(FSOUND_FREE, (CHAR *)pData, FSOUND_LOADMEMORY, 0, Size);
    //int error = g_fn_FSOUND_GetError();
    if (sample)
    {    
        g_fn_FSOUND_PlaySound(FSOUND_FREE, sample);
    }
    else
    {
        // Fall back to WAV sound playback
        sample = (VOID *)-1;
		sndPlaySoundW((WCHAR *)pData, SND_NODEFAULT | SND_ASYNC | SND_MEMORY);
    }

    return sample;
}

VOID *FMOD_PlayFile(WCHAR *pFileName)
{
    if (!InitSoundLibrary()) return NULL;

    CHAR *fileNameA = ToAnsi(pFileName);
    VOID *sample = g_fn_FSOUND_Sample_Load(FSOUND_FREE, fileNameA, FSOUND_LOOP_OFF, 0, 0);
    MEM_FREE(fileNameA);
    fileNameA = NULL;

    if (sample)
    {    
        g_fn_FSOUND_PlaySound(FSOUND_FREE, sample);
    }
    else
    {
        sndPlaySoundW(pFileName, SND_NODEFAULT | SND_ASYNC);
        sample = (VOID *)-1;
    }

    return sample;
}

VOID FMOD_Stop(VOID *pSound)
{
    if (!InitSoundLibrary()) return;

    if (pSound == (VOID *)-1)
    {
        sndPlaySoundW(NULL, 0);
    }
    else
    {
        g_fn_FSOUND_Sample_Free(pSound);
    }
}

BOOL FMOD_IsPlaying()
{
    if (!InitSoundLibrary()) return FALSE;

    INT maxChannels, i;

    maxChannels = g_fn_FSOUND_GetMaxChannels();

    for (i = 0; i < maxChannels; i++)
    {
        if (g_fn_FSOUND_IsPlaying(i))
        {
            return TRUE;
        }
    }

    return FALSE;
}
