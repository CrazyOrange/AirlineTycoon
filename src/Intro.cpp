﻿//============================================================================================
// Intro.cpp : Der Render-Intro
//============================================================================================
#include "StdAfx.h"
#include "Intro.h"
#include "gltitel.h"
#include <cmath>
#include <smacker.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern SB_CColorFX ColorFX;

//--------------------------------------------------------------------------------------------
// ULONG PlayerNum
//--------------------------------------------------------------------------------------------
CIntro::CIntro(BOOL bHandy, SLONG PlayerNum) : CStdRaum(bHandy, PlayerNum, "", 0) {
    RoomBm.ReSize(640, 480);
    RoomBm.FillWith(0);
    PrimaryBm.BlitFrom(RoomBm);

    FrameNum = 0;
    FrameNext = 0;
    bWasIntroPlayed = false;

    StopMidi();
    SSE::DisableDS();

    gMouseStartup = TRUE;

    pGfxMain->LoadLib(const_cast<char *>((LPCTSTR)FullFilename("titel.gli", RoomPath)), &pRoomLib, L_LOCMEM);
    FadeFrom.ReSize(640, 480);
    FadeFrom.FillWith(0);
    FadeTo.ReSize(pRoomLib, GFX_TITEL);

    if (IntroPath.GetLength() != 0) {
        pSmack = smk_open_file(FullFilename("intro.smk", IntroPath), SMK_MODE_MEMORY);
        smk_enable_video(pSmack, 1U);

        smk_info_video(pSmack, &Width, &Height, &Scale);
        if (Scale != SMK_FLAG_Y_NONE) {
            Height *= 2;
        }

        unsigned char tracks = 0;
        unsigned char channels[7];
        unsigned char depth[7];
        unsigned long rate[7];
        smk_enable_audio(pSmack, 0, 1U);
        smk_info_audio(pSmack, &tracks, channels, depth, rate);

        SDL_AudioSpec desired;
        desired.freq = rate[0];
        desired.format = SDL_AUDIO_MASK_SIGNED | (depth[0] & SDL_AUDIO_MASK_BITSIZE);
        desired.channels = channels[0];
        desired.samples = 2048;
        desired.callback = nullptr;
        desired.userdata = nullptr;
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        if (audioDevice == 0U) {
            Hdu.HercPrintf(SDL_GetError());
        }

        if (pSmack != nullptr) {
            bWasIntroPlayed = true;
        }

        State = smk_first(pSmack);
        Bitmap.ReSize(XY(Width, Height), CREATE_SYSMEM | CREATE_INDEXED);
        {
            // Copy video frame with line-doubling if needed
            SB_CBitmapKey Key(*Bitmap.pBitmap);
            const unsigned char *pVideo = smk_get_video(pSmack);
            int scale_mode = Scale == SMK_FLAG_Y_NONE ? 1 : 2;
            for (unsigned long y = 0; y < Height; y++) {
                memcpy(static_cast<BYTE *>(Key.Bitmap) + (y * Key.lPitch), pVideo + ((y / scale_mode) * Key.lPitch), Key.lPitch);
            }
        }
        CalculatePalettemapper(smk_get_palette(pSmack), Bitmap.pBitmap->GetPixelFormat()->palette);
        SDL_QueueAudio(audioDevice, smk_get_audio(pSmack, 0), smk_get_audio_size(pSmack, 0));
        State = smk_next(pSmack);
    } else {
        audioDevice = 0;
        pSmack = nullptr;
        Sim.Gamestate = GAMESTATE_BOOT;
    }

    SDL_ShowWindow(FrameWnd->m_hWnd);
    SDL_UpdateWindowSurface(FrameWnd->m_hWnd);
}

//--------------------------------------------------------------------------------------------
// CIntro-Fenster zerstören:
//--------------------------------------------------------------------------------------------
CIntro::~CIntro() {
    if (bWasIntroPlayed) {
        Sim.Options.OptionViewedIntro = TRUE;
    }

    FadeFrom.Destroy();
    FadeTo.Destroy();

    if ((pRoomLib != nullptr) && (pGfxMain != nullptr)) {
        pGfxMain->ReleaseLib(pRoomLib);
        pRoomLib = nullptr;
    }

    if (audioDevice != 0U) {
        SDL_CloseAudioDevice(audioDevice);
    }
    audioDevice = 0;

    if (pSmack != nullptr) {
        smk_close(pSmack);
    }
    pSmack = nullptr;

    gMouseStartup = FALSE;
    pCursor->SetImage(gCursorBm.pBitmap);

    if (Sim.Options.OptionEnableDigi != 0) {
        gpSSE->EnableDS();
    }
    if (Sim.Options.OptionMusicType != 0) {
        NextMidi();
    }
    SetMidiVolume(Sim.Options.OptionMusik);
}

/////////////////////////////////////////////////////////////////////////////
// CIntro message handlers

//--------------------------------------------------------------------------------------------
// void CIntro::OnPaint():
//--------------------------------------------------------------------------------------------
void CIntro::OnPaint() {
    if (FrameNum++ < 2) {
        PrimaryBm.BlitFrom(RoomBm);
    }

    // Die Standard Paint-Sachen kann der Basisraum erledigen
    CStdRaum::OnPaint();

    SDL_PauseAudioDevice(audioDevice, 0);

    if (pSmack != nullptr) {
        if (timeGetTime() >= FrameNext && State == SMK_MORE) {
            // Take the next frame:
            Bitmap.ReSize(XY(Width, Height), CREATE_SYSMEM | CREATE_INDEXED);
            {
                // Copy video frame with line-doubling if needed
                SB_CBitmapKey Key(*Bitmap.pBitmap);
                const unsigned char *pVideo = smk_get_video(pSmack);
                int scale_mode = Scale == SMK_FLAG_Y_NONE ? 1 : 2;
                for (unsigned long y = 0; y < Height; y++) {
                    memcpy(static_cast<BYTE *>(Key.Bitmap) + (y * Key.lPitch), pVideo + ((y / scale_mode) * Key.lPitch), Key.lPitch);
                }
            }
            CalculatePalettemapper(smk_get_palette(pSmack), Bitmap.pBitmap->GetPixelFormat()->palette);
            SDL_QueueAudio(audioDevice, smk_get_audio(pSmack, 0), smk_get_audio_size(pSmack, 0));
            State = smk_next(pSmack);

            double usf = NAN;
            smk_info_all(pSmack, nullptr, nullptr, &usf);
            FrameNext = timeGetTime() + (usf / 1000.0);
        }

        PrimaryBm.BlitFrom(Bitmap, 320 - Width / 2, 240 - Height / 2);

        if (State != SMK_MORE) {
            if (Sim.Options.OptionViewedIntro == 0) {
                FadeFrom.BlitFrom(Bitmap, 320 - Width / 2, 240 - Height / 2);

                FadeCount = timeGetTime();

                smk_close(pSmack);
                pSmack = nullptr;
            } else {
                Sim.Gamestate = GAMESTATE_BOOT;
            }
        }
    } else {
        if (timeGetTime() - FadeCount < 1000) {
            SLONG Level = min(1000, timeGetTime() - FadeCount) * 8 / 1000;
            ColorFX.ApplyOn2(Level, FadeTo.pBitmap, 8 - Level, FadeFrom.pBitmap, &PrimaryBm.PrimaryBm);
        } else {
            PrimaryBm.BlitFrom(FadeTo);

            if (timeGetTime() - FadeCount > 3000) {
                Sim.Gamestate = GAMESTATE_BOOT;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
// void CIntro::OnLButtonDown(UINT nFlags, CPoint point)
//--------------------------------------------------------------------------------------------
void CIntro::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/) {
    if ((pSmack != nullptr) && Sim.Options.OptionViewedIntro == 0) {
        FadeFrom.BlitFrom(Bitmap, 320 - Width / 2, 240 - Height / 2);

        FadeCount = timeGetTime();

        smk_close(pSmack);
        pSmack = nullptr;
    } else {
        Sim.Gamestate = GAMESTATE_BOOT;
    }
}

//--------------------------------------------------------------------------------------------
// void CIntro::OnRButtonDown(UINT nFlags, CPoint point):
//--------------------------------------------------------------------------------------------
void CIntro::OnRButtonDown(UINT /*nFlags*/, CPoint /*point*/) {
    DefaultOnRButtonDown();

    if ((pSmack != nullptr) && Sim.Options.OptionViewedIntro == 0) {
        FadeFrom.BlitFrom(Bitmap, 320 - Width / 2, 240 - Height / 2);

        FadeCount = timeGetTime();

        smk_close(pSmack);
        pSmack = nullptr;
    } else {
        Sim.Gamestate = GAMESTATE_BOOT;
    }
}

//--------------------------------------------------------------------------------------------
// BOOL CStdRaum::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) : AG:
//--------------------------------------------------------------------------------------------
BOOL CIntro::OnSetCursor(void *pWnd, UINT nHitTest, UINT message) { return (FrameWnd->OnSetCursor(pWnd, nHitTest, message)); }

//--------------------------------------------------------------------------------------------
// void CStdRaum::OnMouseMove(UINT nFlags, CPoint point): AG:
//--------------------------------------------------------------------------------------------
void CIntro::OnMouseMove(UINT nFlags, CPoint point) { FrameWnd->OnMouseMove(nFlags, point); }

//--------------------------------------------------------------------------------------------
// void CIntro::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//--------------------------------------------------------------------------------------------
void CIntro::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/) {
    if (nChar == VK_ESCAPE) {
        if ((pSmack != nullptr) && Sim.Options.OptionViewedIntro == 0) {
            FadeFrom.BlitFrom(Bitmap, 320 - Width / 2, 240 - Height / 2);

            FadeCount = timeGetTime();

            smk_close(pSmack);
            pSmack = nullptr;
        } else {
            Sim.Gamestate = GAMESTATE_BOOT;
        }
    }
}