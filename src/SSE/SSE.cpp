#include "StdAfx.h"
#include <vector>
#include <algorithm>

SSE::SSE(void* hWnd, dword samplesPerSec, word channels, word bitsPerSample, word maxFX)
    : _hWnd((SDL_Window*)hWnd)
    , _samplesPerSec(samplesPerSec)
    , _channels(channels)
    , _bitsPerSample(bitsPerSample)
    , _maxSound(maxFX)
    , _fSoundEnabled(true)
      , _fMusicEnabled(true)
{
}

SSE::~SSE()
{
    DisableDS();
}

int SSE::EnableDS() const
{
    if (Mix_OpenAudioDevice(_samplesPerSec, SDL_AUDIO_MASK_SIGNED | (_bitsPerSample & SDL_AUDIO_MASK_BITSIZE), _channels, 1024, nullptr, 0) < 0)
    {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return SSE_DSOUND_NOINIT;
    }

    Mix_AllocateChannels(_maxSound);
    return SSE_OK;
}

int SSE::DisableDS()
{
    Mix_HookMusicFinished(nullptr);
    Mix_CloseAudio();
    return SSE_OK;
}

int SSE::CreateFX(FX** ppFX, char* file, dword samplesPerSec, word channels, word bitsPerSample)
{
    if (ppFX == nullptr) {
        return SSE_INVALIDPARAM;
}

    _soundObjList.emplace_back(FX());
    *ppFX = &_soundObjList.back();
    return (*ppFX)->Create(this, file, samplesPerSec, channels, bitsPerSample);
}

int SSE::CreateMidi(MIDI** ppMidi, char* file)
{
    if (ppMidi == nullptr) {
        return SSE_INVALIDPARAM;
}

    _musicObjList.emplace_back(MIDI());
    *ppMidi = &_musicObjList.back();
    return (*ppMidi)->Create(this, file);
}

int SSE::EnableSound(bool fSoundEnabled)
{
    if (fSoundEnabled == _fSoundEnabled) {
        return SSE_OK;
}
    _fSoundEnabled = fSoundEnabled;
    if (!_fSoundEnabled) {
        StopSound();
}
    return SSE_OK;
}

void SSE::StopSound()
{
    for (FX& fx : _soundObjList) {
        fx.Stop();
}
}

int SSE::EnableMusic(bool fMusicEnabled)
{
    if (fMusicEnabled == _fMusicEnabled) {
        return SSE_OK;
}
    _fMusicEnabled = fMusicEnabled;
    if (!_fMusicEnabled) {
        StopMusic();
}
    return SSE_OK;
}

void SSE::StopMusic()
{
    for (MIDI& mid : _musicObjList) {
        mid.Stop();
}
}

int SSE::SetMusicVolume(SLONG volume)
{
    volume = min(MIX_MAX_VOLUME, (MIX_MAX_VOLUME / 7.0F) * volume);

    Mix_VolumeMusic(volume);
    return SSE_OK;
}

int SSE::GetMusicVolume(SLONG* pVolume)
{
    if (pVolume == nullptr) {
        return SSE_INVALIDPARAM;
}

    *pVolume = Mix_VolumeMusic(-1);
    return SSE_OK;
}

int SSE::SetSoundVolume(SLONG volume)
{
    volume = min(MIX_MAX_VOLUME, (MIX_MAX_VOLUME / 7.0F) * volume);

    Mix_Volume(-1, volume);
    return SSE_OK;
}

int SSE::GetSoundVolume(SLONG* pVolume)
{
    if (pVolume == nullptr) {
        return SSE_INVALIDPARAM;
}

    *pVolume = Mix_Volume(-1, -1);
    return SSE_OK;
}

void SSE::SetMusicCallback(void (*callback)())
{
    Mix_HookMusicFinished(callback);
}

word SSE::GetSoundPlaying()
{
    return Mix_Playing(-1);
}

FX::FX()
{
    memset(&_fxData, 0, sizeof(_fxData));
}

FX::~FX()
= default;

int	FX::Create(SSE* pSSE, char* file, dword samplesPerSec, word channels, word bitsPerSample)
{
    _digitalData.pSSE = pSSE;
    SetFormat(samplesPerSec, channels, bitsPerSample);

    if (file != nullptr) {
        Load(file);
}

    return SSE_OK;
}

bool FX::StopPriority(dword flags)
{
    if (((flags & DSBPLAY_PRIORITY) == 0u) && ((flags & DSBPLAY_HIGHPRIORITY) == 0u)) {
        return false;
}

    FX* playing = nullptr;
    for (FX& fx : _digitalData.pSSE->_soundObjList)
    {
        dword status = 0;
        fx.GetStatus(&status);
        if ((status & DSBSTATUS_PLAYING) != 0u)
        {
            if (!fx._digitalData.fNoStop)
            {
                fx.Stop();
                return true;
            }
            if (playing == nullptr) {
                playing = &fx;
}
        }
    }

    if ((playing != nullptr) && ((flags & DSBPLAY_HIGHPRIORITY) != 0u))
    {
        playing->Stop();
        return true;
    }
    return false;
}

SLONG FX::Release()
{
    Free();

    if (_digitalData.pSSE != nullptr)
    {
        std::list<FX>& list = _digitalData.pSSE->_soundObjList;
        auto it = std::find_if(list.begin(), list.end(), [this](const FX& fx) { return &fx == this; });
        if (it != list.end()) {
            list.erase(it);
}
    }
    return 0;
}

int FX::Play(dword dwFlags, SLONG pan)
{
    if (!_digitalData.pSSE->IsSoundEnabled()) {
        return SSE_SOUNDDISABLED;
}

    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    // TODO: Panning
    if ((dwFlags & DSBPLAY_SETPAN) != 0u) {
        SetPan(pan);
}

    if (Mix_Playing(-1) >= _digitalData.pSSE->_maxSound
            && !StopPriority(dwFlags)) {
        return SSE_MAXFXREACHED;
}

    if (_digitalData.fNoStop)
    {
        dword status = 0;
        GetStatus(&status);
        if ((status & DSBSTATUS_PLAYING) != 0u) {
            return SSE_OK;
}
    }
    _digitalData.fNoStop = ((dwFlags & DSBPLAY_NOSTOP) != 0u);

    if ((dwFlags & DSBPLAY_FIRE) == 0u) {
        Stop();
}

    _digitalData.time = timeGetTime();
    return Mix_PlayChannel(-1, _fxData.pBuffer, dwFlags & DSBPLAY_LOOPING ? -1 : 0) < 0 ? SSE_CANNOTPLAY : SSE_OK;
}

int FX::Stop()
{
    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    for (int i = 0; i < _digitalData.pSSE->_maxSound; i++)
    {
        if (Mix_GetChunk(i) == _fxData.pBuffer) {
            Mix_HaltChannel(i);
}
    }
    _digitalData.time = 0;
    return SSE_OK;
}

int FX::Pause()
{
    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    for (int i = 0; i < _digitalData.pSSE->_maxSound; i++)
    {
        if (Mix_GetChunk(i) == _fxData.pBuffer) {
            Mix_Pause(i);
}
    }
    _digitalData.time = timeGetTime() - _digitalData.time;
    return SSE_OK;
}

int FX::Resume()
{
    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    for (int i = 0; i < _digitalData.pSSE->_maxSound; i++)
    {
        if (Mix_GetChunk(i) == _fxData.pBuffer) {
            Mix_Resume(i);
}
    }
    _digitalData.time = timeGetTime() - _digitalData.time;
    return SSE_OK;
}

int FX::GetVolume(SLONG* pVolume)
{
    if (pVolume == nullptr) {
        return SSE_INVALIDPARAM;
}

    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    *pVolume = Mix_VolumeChunk(_fxData.pBuffer, -1);
    return SSE_OK;
}

int FX::SetVolume(SLONG volume)
{
    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    Mix_VolumeChunk(_fxData.pBuffer, volume);
    return SSE_OK;
}

int FX::GetPan(SLONG* pPan)
{
    if (pPan == nullptr) {
        return SSE_INVALIDPARAM;
}

    return SSE_OK;
}

int FX::SetPan(SLONG pan)
{
    return SSE_OK;
}

int ChangeFrequency(Mix_Chunk* chunk, int freq) {

    Uint16 format = 0;
    int channels = 0;
    Mix_QuerySpec(nullptr, &format, &channels);

    int channel = 0;
    SDL_AudioCVT cvt;

    //Create a converter from the PLAY_FREQUENCY to "freq"
    SDL_BuildAudioCVT(&cvt, format, channels, freq, format, channels, 44100);

    if (cvt.needed != 0) { //If need to convert

        for (channel = 0; channel < MIX_CHANNELS; channel++) {
            if (Mix_Playing(channel) == 0) { break; //Find a free channel
}
}

        if (channel == MIX_CHANNELS) { return -1; //If there is no channel return -1
}

        //Set converter lenght and buffer
        cvt.len = chunk->alen;
        cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);
        if (cvt.buf == nullptr) { return -1;
}

        //Copy the Mix_Chunk data to the new chunk and make the conversion
        SDL_memcpy(cvt.buf, chunk->abuf, chunk->alen);
        if (SDL_ConvertAudio(&cvt) < 0) {
            SDL_free(cvt.buf);
            return -1;
        }

        //If it was sucessfull put it on the original Mix_Chunk
        chunk->abuf = cvt.buf;
        chunk->alen = cvt.len_cvt;

        return channel;

    }

    return -1;
}


int FX::Load(const char* file)
{
    if (_fxData.pBuffer != nullptr) {
        Free();
}

    _digitalData.file = file;
    auto* buf = (Uint8*)SDL_LoadFile(file, &_fxData.bufferSize);
    _fxData.pBuffer = Mix_QuickLoad_RAW(buf, _fxData.bufferSize);
    ChangeFrequency(_fxData.pBuffer, 22050);
    _fxData.bufferSize = _fxData.pBuffer->alen;
    return SSE_OK;
}

int FX::Fusion(const FX** Fx, SLONG NumFx)
{
    for (SLONG i = 0; i < NumFx; i++)
    {
        if ((Fx[i] == nullptr) || (Fx[i]->_fxData.pBuffer == nullptr)) {
            return SSE_INVALIDPARAM;
}
    }

    Free();

    for (SLONG i = 0; i < NumFx; i++) {
        _fxData.bufferSize += Fx[i]->_fxData.bufferSize;
}
    auto* buf = (Uint8*)SDL_malloc(_fxData.bufferSize);
    size_t pos = 0;
    for (SLONG i = 0; i < NumFx; i++)
    {
        memcpy(buf + pos, Fx[i]->_fxData.pBuffer->abuf, Fx[i]->_fxData.bufferSize);
        pos += Fx[i]->_fxData.bufferSize;
    }
    _fxData.pBuffer = Mix_QuickLoad_RAW(buf, _fxData.bufferSize);
    return SSE_OK;
}

int FX::Fusion(const FX* Fx, SLONG* Von, SLONG* Bis, SLONG NumFx)
{
    if ((Fx == nullptr) || (Fx->_fxData.pBuffer == nullptr)) {
        return SSE_INVALIDPARAM;
}

    Free();

    for (SLONG i = 0; i < NumFx; i++) {
        _fxData.bufferSize += Bis[i] - Von[i];
}
    auto* buf = (Uint8*)SDL_malloc(_fxData.bufferSize);
    size_t pos = 0;
    for (SLONG i = 0; i < NumFx; i++)
    {
        memcpy(buf + pos, Fx->_fxData.pBuffer->abuf + Von[i], Bis[i] - Von[i]);
        pos += Bis[i] - Von[i];
    }
    _fxData.pBuffer = Mix_QuickLoad_RAW(buf, _fxData.bufferSize);
    return SSE_OK;
}

int FX::Tokenize(__int64 Token, SLONG* Von, SLONG* Bis, SLONG& rcAnzahl)
{
    if ((_fxData.pBuffer == nullptr) || _fxData.bufferSize < sizeof(__int64)) {
        return SSE_NOSOUNDLOADED;
}

    size_t count = 0;
    size_t i = 0;
    Von[count++] = 0;
    Uint8* ptr = _fxData.pBuffer->abuf;
    for (i = 0; i < _fxData.bufferSize - 7; i++)
    {
        if (*(__int64*)ptr == Token)
        {
            Bis[count - 1] = ((i - 1) & 0xFFFFFE) + 2;
            Von[count++] = (i + 8) & 0xFFFFFE;
        }
    }
    Bis[count - 1] = (i - 1);
    rcAnzahl = count;
    return SSE_OK;
}

FX** FX::Tokenize(__int64 Token, SLONG& rcAnzahl)
{
    if ((_fxData.pBuffer == nullptr) || _fxData.bufferSize < sizeof(__int64)) {
        return nullptr;
}

    std::vector<size_t> slices;
    Uint8* ptr = _fxData.pBuffer->abuf;
    for (size_t i = 0; i < _fxData.bufferSize - 7; i++)
    {
        if (*(__int64*)ptr == Token) {
            slices.push_back(i);
}
    }
    slices.push_back(_fxData.bufferSize);

    FX** pFX = new FX*[slices.size()];
    size_t pos = 0;
    for (size_t i = 0; i < slices.size(); i++)
    {
        _digitalData.pSSE->CreateFX(&pFX[i]);

        size_t size = slices[i] - pos;
        auto* buf = (Uint8*)SDL_malloc(size);
        memcpy(buf, _fxData.pBuffer->abuf + pos, size);
        pFX[i]->_fxData.bufferSize = size;
        pFX[i]->_fxData.pBuffer = Mix_QuickLoad_RAW(buf, size);
        pos = slices[i];
    }
    rcAnzahl = slices.size();
    return pFX;
}

int FX::Free()
{
    if (_fxData.pBuffer != nullptr)
    {
        Stop();
        void* buf = _fxData.pBuffer->abuf;
        Mix_FreeChunk(_fxData.pBuffer);
        SDL_free(buf);
    }
    _digitalData.file.clear();
    _fxData.bufferSize = 0;
    return SSE_OK;
}

int FX::GetStatus(dword* pStatus)
{
    if (pStatus == nullptr) {
        return SSE_INVALIDPARAM;
}

    if (_fxData.pBuffer == nullptr) {
        return SSE_NOSOUNDLOADED;
}

    *pStatus = 0;
    for (int i = 0; i < _digitalData.pSSE->_maxSound; i++)
    {
        if (Mix_GetChunk(i) == _fxData.pBuffer)
        {
            if (Mix_Playing(i) != 0) {
                *pStatus |= DSBSTATUS_PLAYING;
}
        }
    }

    return SSE_OK;
}

bool FX::IsMouthOpen(SLONG PreTime)
{
    if ((_fxData.pBuffer == nullptr) || (_digitalData.time == 0u)) {
        return false;
}

    dword pos = 22050 * (timeGetTime() - _digitalData.time + PreTime) / 1000;
    if (pos * sizeof(Uint16) + 2000 >= _fxData.bufferSize) {
        return false;
}

    Uint16* sampleBuf = ((Uint16*)_fxData.pBuffer->abuf) + pos;
    return (*sampleBuf) > 512
        || (sampleBuf[100]) > 512
        || (sampleBuf[200]) > 512
        || (sampleBuf[400]) > 512
        || (sampleBuf[560]) > 512
        || (sampleBuf[620]) > 512
        || (sampleBuf[700]) > 512
        || (sampleBuf[800]) > 512
        || (sampleBuf[900]) > 512
        || (sampleBuf[999]) > 512;
}

word FX::CountPlaying()
{
    word count = 0;
    for (int i = 0; i < _digitalData.pSSE->_maxSound; i++)
    {
        if (Mix_GetChunk(i) == _fxData.pBuffer)
        {
            if (Mix_Playing(i) != 0) {
                count++;
}
        }
    }
    return count;
}

void FX::SetFormat(dword samplesPerSec, word channels, word bitsPerSample)
{
    dword lastSamplesPerSec = _fxData.samplesPerSec;
    word lastChannels = _fxData.channels;
    word lastBitsPerSample = _fxData.bitsPerSample;

    _fxData.samplesPerSec = samplesPerSec != 0u ? samplesPerSec : _digitalData.pSSE->_samplesPerSec;
    _fxData.channels = channels != 0u ? channels : 1;
    _fxData.bitsPerSample = bitsPerSample != 0u ? bitsPerSample : _digitalData.pSSE->_bitsPerSample;

    if ((_fxData.pBuffer != nullptr) && (_fxData.samplesPerSec != lastSamplesPerSec ||
                _fxData.channels != lastChannels || _fxData.bitsPerSample != lastBitsPerSample))
    {
        Free();
        if (!_digitalData.file.empty()) {
            Load(_digitalData.file.c_str());
}
    }
}

MIDI::MIDI()
{
    _music = nullptr;
}

MIDI::~MIDI()
= default;

int MIDI::Create(SSE* pSSE, char* file)
{
    _musicData.pSSE = pSSE;

    if (file != nullptr) {
        Load(file);
}

    return SSE_OK;
}

void MIDI::SetMode(int mode) {
    _mode = mode;
}

bool MIDI::StopPriority(dword flags)
{
    return false;
}

SLONG MIDI::Release()
{
    Free();

    if (_musicData.pSSE != nullptr)
    {
        std::list<MIDI>& list = _musicData.pSSE->_musicObjList;
        auto it = std::find_if(list.begin(), list.end(), [this](const MIDI& mid) { return &mid == this; });
        if (it != list.end()) {
            list.erase(it);
}
    }
    return 0;
}

int MIDI::Play(dword dwFlags, SLONG pan)
{
    if (!_musicData.pSSE->IsMusicEnabled()) {
        return SSE_MUSICDISABLED;
}

    if (_music == nullptr) {
        return SSE_NOMUSICLOADED;
}

    // TODO: Panning
    if ((dwFlags & DSBPLAY_SETPAN) != 0u) {
        SetPan(pan);
}

    return Mix_PlayMusic(_music, 0) < 0 ? SSE_CANNOTPLAY : SSE_OK;
}

int MIDI::Stop()
{
    return Mix_HaltMusic() < 0 ? SSE_CANNOTPLAY : SSE_OK;
}

int MIDI::Pause()
{
    Mix_PauseMusic();
    return SSE_OK;
}

int MIDI::Resume()
{
    Mix_ResumeMusic();
    return SSE_OK;
}

int MIDI::GetVolume(SLONG* pVolume)
{
    *pVolume = Mix_VolumeMusic(-1);
    return SSE_OK;
}

int MIDI::SetVolume(SLONG volume)
{
    Mix_VolumeMusic(volume);
    return SSE_OK;
}

int MIDI::GetPan(SLONG* pPan)
{
    return SSE_OK;
}

int MIDI::SetPan(SLONG pan)
{
    return SSE_OK;
}

int MIDI::Load(const char* file)
{
    if (_music != nullptr) {
        Free();
}

    _musicData.file = file;

    if(_mode == 1){
        _music = Mix_LoadMUS(_musicData.file.c_str());
        // Some versions ship with ogg music as well, use it as a fall-back
    } else if(_mode == 2){
        _musicData.file.replace(_musicData.file.size() - 3, 3, "ogg");
        _music = Mix_LoadMUS(_musicData.file.c_str());
    }

    if(_music == nullptr)
    {
        printf("MP: Could not load music: %s\n", _musicData.file.c_str());
    }

    _musicData.pSSE->_playingMusicObj = this;
    return SSE_OK;
}

int MIDI::Free()
{
    Mix_FreeMusic(_music);
    return SSE_OK;
}

int MIDI::GetStatus(dword* pStatus)
{
    return SSE_OK;
}

word MIDI::CountPlaying()
{
    return Mix_PlayingMusic();
}
