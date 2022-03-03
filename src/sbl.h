#pragma once

#include <list>
#include <map>

typedef unsigned short word;
typedef unsigned int dword;

// Can you spot the bug? (x is executed two more times just to get the error codes)
// Bonus points if you spot that FAILED() should've been used to check the HRESULT.
#define DD_ERROR(x)                                                                                                                                            \
    if (!(x))                                                                                                                                                  \
        ODS("DDError in File: %s Line: %d Code: %d [%x]", __FILE__, __LINE__, x, x);

extern void ODS(const char *, ...);
extern SLONG GetLowestSetBit(SLONG mask);
extern SLONG GetHighestSetBit(SLONG mask);

#define CREATE_SYSMEM 0
#define CREATE_VIDMEM 1
#define CREATE_USECOLORKEY 2
#define CREATE_USEZBUFFER 4
#define CREATE_USEALPHA 8
#define CREATE_FULLSCREEN 16
#define CREATE_INDEXED 32

class GfxLib {
  public:
    GfxLib(void *, SDL_Renderer *, char *, SLONG, SLONG, SLONG *);

    struct _GfxStruct *ReloadSurface(__int64);
    static SLONG Restore(void);
    void Release(void);
    class GfxLib *ReleaseSurface(SLONG);
    class GfxLib *ReleaseSurface(__int64);
    SDL_Surface *GetSurface(__int64);
    SDL_Surface *GetSurface(SLONG);
    static SLONG AddRef(__int64);
    SLONG AddRef(SLONG);
    __int64 LongName2Id(char *);
    char *Id2LongName(__int64);
    SLONG GetGfxHeader(SLONG, struct _GfxChunkInfo *);
    __int64 GetGfxShortId(SLONG);

  protected:
    void ErrorProc(SLONG);

    friend class GfxMain;

  private:
    SLONG CountGfxChunks(struct _uniChunk *, SLONG);
    static struct _GfxLibHeader *LoadHeader(SDL_RWops *);
    void ReadPaletteChunk(int, struct _PaletteInfo);
    void ReadNameChunk(int, struct _LongNameChunk);
    void *DeCompData(void *, struct _GfxChunkInfo, SLONG);
    void *ConvertData(void *, SLONG, char *, SLONG, SLONG, SLONG, SLONG, SLONG);
    struct IDirectDrawSurface *FillSurface(int, struct _GfxChunkInfo, char *, struct IDirectDrawSurface *);
    struct IDirectDrawSurface *ReadPixelData(int, struct _GfxChunkInfo, char *, SLONG);
    struct IDirectDrawPalette *ReadPalette(int, struct _GfxChunkInfo);
    void *ReadZBuffer(SDL_RWops *, struct _GfxChunkInfo);
    void *ReadAlphaBuffer(SDL_RWops *, struct _GfxChunkInfo);
    SLONG ReadGfxChunk(SDL_RWops *, struct _GfxChunkHeader, SLONG, SLONG);
    SLONG Load(SDL_RWops *, struct _GfxLibHeader *);
    SLONG FindId(__int64);
    void RelSurface(SLONG);

    std::map<__int64, SDL_Surface *> Surfaces;
    CString Path;
};

#define L_LOCMEM 0

class GfxMain {
  public:
    GfxMain(SDL_Renderer *);
    ~GfxMain(void);
    GfxMain(const GfxMain&) = delete;
    GfxMain& operator=(const GfxMain&) = delete;

    SLONG Restore(void);
    SLONG LoadLib(char *, class GfxLib **, SLONG);
    SLONG ReleaseLib(class GfxLib *);
    void KillLib(class GfxLib *);
    SLONG GetListId(class GfxLib *, __int64);
    struct _GfxStruct *GetSurface(SLONG);
    SLONG AddRef(SLONG);
    class GfxLib *ReleaseSurface(SLONG);

  private:
    void ErrorProc(SLONG);

    std::list<GfxLib> Libs;
};

struct SB_Hardwarecolor {
    word Color;

    SB_Hardwarecolor(word c = 0) : Color(c) {}
    operator word() { return Color; }
};

class SB_CBitmapCore {
  public:
    SB_CBitmapCore() = default;
    SB_CBitmapCore(SLONG id) : Id(id) { }

    ULONG AddAlphaMsk(void);
    ULONG AddZBuffer(ULONG, ULONG);
    SB_Hardwarecolor GetHardwarecolor(ULONG);
    ULONG SetPixel(SLONG, SLONG, SB_Hardwarecolor);
    ULONG GetPixel(SLONG, SLONG);
    ULONG Clear(SB_Hardwarecolor, struct tagRECT const * = NULL);
    ULONG Line(SLONG, SLONG, SLONG, SLONG, SB_Hardwarecolor);
    ULONG LineTo(SLONG, SLONG, SB_Hardwarecolor);
    ULONG Rectangle(const RECT *, SB_Hardwarecolor);
    void InitClipRect(void);
    void SetClipRect(const CRect &);
    void SetColorKey(ULONG);
    virtual ULONG Release(void);
    ULONG BlitFast(class SB_CBitmapCore *, SLONG, SLONG);
    ULONG BlitFast(class SB_CBitmapCore *, SLONG, SLONG, const CRect &);
    ULONG BlitChar(SDL_Surface *, SLONG, SLONG, const SDL_Rect &);
    ULONG Blit(class SB_CBitmapCore *, SLONG, SLONG);
    ULONG Blit(class SB_CBitmapCore *, SLONG, SLONG, const CRect &);
    SLONG BlitA(class SB_CBitmapCore *, SLONG, SLONG, const RECT *, SB_Hardwarecolor);
    SLONG BlitA(class SB_CBitmapCore *, SLONG, SLONG, const RECT *);
    SLONG BlitAT(class SB_CBitmapCore *, SLONG, SLONG, const RECT *, SB_Hardwarecolor);
    SLONG BlitAT(class SB_CBitmapCore *, SLONG, SLONG, const RECT *);

    ULONG BlitT(class SB_CBitmapCore *bm, SLONG x, SLONG y) { return Blit(bm, x, y); }
    ULONG BlitT(class SB_CBitmapCore *bm, SLONG x, SLONG y, const CRect &rect) { return Blit(bm, x, y, rect); }
    ULONG SetPixel(SLONG x, SLONG y, SLONG color) { return SetPixel(x, y, GetHardwarecolor(color)); }
    ULONG Line(SLONG x1, SLONG y1, SLONG x2, SLONG y2, DWORD color) { return Line(x1, y1, x2, y2, GetHardwarecolor(color)); }
    SLONG GetXSize() { return Size.x; }
    SLONG GetYSize() { return Size.y; }
    CRect GetClipRect() {
        const SDL_Rect &r = lpDDSurface->clip_rect;
        return CRect(r.x, r.y, r.x + r.w, r.y + r.h);
    }
    SDL_Surface *GetSurface() { return lpDDSurface; }
    SDL_Surface *GetFlippedSurface();
    SDL_PixelFormat *GetPixelFormat(void) { return lpDDSurface->format; }
    SDL_Texture *GetTexture() { return lpTexture; }

    SLONG GetRef() const { return RefCounter; }
    void IncRef() { ++RefCounter; }
    bool DecRef() { assert(RefCounter > 0); return (0 == --RefCounter); }
    SLONG getId() const { return Id; }

    friend class SB_CBitmapMain;
    friend class SB_CBitmapKey;

  protected:
    SDL_Renderer *lpDD{nullptr};
    SDL_Surface *lpDDSurface{nullptr};
    SDL_Surface *flippedBufferSurface{nullptr};
    SDL_Texture *lpTexture{nullptr};
    XY Size;

  private:
    SLONG Id{-1};
    SLONG RefCounter{0};
};

// static_assert(sizeof(SB_CBitmapCore) == 0x5Cu, "SB_CBitmapCore size check");

class SB_CCursor {
  public:
    SB_CCursor(class SB_CPrimaryBitmap *, class SB_CBitmapCore * = NULL);
    ~SB_CCursor(void);
    SB_CCursor(const SB_CCursor&) = delete;
    SB_CCursor& operator=(const SB_CCursor&) = delete;

    SLONG Create(class SB_CBitmapCore *);
    SLONG SetImage(class SB_CBitmapCore *);
    SLONG MoveImage(SLONG, SLONG);
    SLONG FlipBegin();
    SLONG FlipEnd();
    SLONG Show(bool);
    SLONG Render(SDL_Renderer *);

  private:
    SLONG BlitImage(SLONG, SLONG);
    SLONG RestoreBackground(struct SDL_Surface *);
    SLONG SaveBackground(struct SDL_Surface *);
    SLONG CreateBackground(void);
    static SLONG CreateSurface(struct SDL_Surface **, SLONG, SLONG);

    SB_CPrimaryBitmap *Primary;
    SB_CBitmapCore *Cursor;
    SDL_Surface *Background;
    XY Position;
    dword Unknown[22]{};
};

// static_assert(sizeof(SB_CCursor) == 0x6Cu, "SB_CCursor size check");

class SB_CPrimaryBitmap : public SB_CBitmapCore {
  public:
    SB_CPrimaryBitmap() = default;
    ~SB_CPrimaryBitmap() { Release(); }

    SLONG Create(SDL_Renderer **, SDL_Window *, unsigned short, SLONG, SLONG, unsigned char, unsigned short);
    virtual ULONG Release(void);
    SLONG Flip(void);
    SLONG Present(void);
    void SetPos(POINT);

    void AssignCursor(SB_CCursor *c) { Cursor = c; }
    SDL_Window *GetPrimarySurface() { return Window; }
    static bool FastClip(CRect clipRect, POINT *pPoint, RECT *pRect);

  private:
    void Delete(void);

    SDL_Window *Window{};
    dword Unknown[9]{};
    SB_CCursor *Cursor{};
};

// static_assert(sizeof(SB_CPrimaryBitmap) == 0x88u, "SB_CPrimaryBitmap size check");

class SB_CBitmapMain {
  public:
    SB_CBitmapMain(SDL_Renderer *);
    ~SB_CBitmapMain(void);
    SB_CBitmapMain(const SB_CBitmapMain&) = delete;
    SB_CBitmapMain& operator=(const SB_CBitmapMain&) = delete;

    ULONG Release(void);
    ULONG CreateBitmap(SB_CBitmapCore **, GfxLib *, __int64, ULONG);
    ULONG CreateBitmap(SB_CBitmapCore **, SLONG, SLONG, ULONG, ULONG = 16, ULONG = 0);
    ULONG ReleaseBitmap(SB_CBitmapCore *);
    ULONG DelEntry(SB_CBitmapCore *);

  private:
    SDL_Renderer *Renderer;
    std::unordered_map<SLONG, SB_CBitmapCore> Bitmaps;
    SLONG UniqueId{0};
    dword Unknown[3]{};
};

// static_assert(sizeof(SB_CBitmapMain) == 0x1Cu, "SB_CBitmapMain size check");

class SB_CBitmapKey {
  public:
    SB_CBitmapKey(class SB_CBitmapCore &);
    ~SB_CBitmapKey(void);
    SB_CBitmapKey(const SB_CBitmapKey&) = delete;
    SB_CBitmapKey& operator=(const SB_CBitmapKey&) = delete;
    SB_CBitmapKey(SB_CBitmapKey&& o) {
        Surface = std::exchange(o.Surface, nullptr);
        Bitmap = std::exchange(o.Bitmap, nullptr);
        lPitch = std::exchange(o.lPitch, 0);
    }
    SB_CBitmapKey& operator=(SB_CBitmapKey&& o) {
        std::swap(Surface, o.Surface);
        std::swap(Bitmap, o.Bitmap);
        std::swap(lPitch, o.lPitch);
        return *this;
    }

    SDL_Surface *Surface;
    dword Unknown[27]{};
    void *Bitmap;
    int lPitch;
};

// static_assert(sizeof(SB_CBitmapKey) == 0x78u, "SB_CBitmapKey size check");

typedef struct {
    SB_CPrimaryBitmap *pBitmap{nullptr};
    SB_CBitmapCore *pBitmapCore{nullptr};
} CFRONTDATA;

#define TEC_FONT_LEFT 1
#define TEC_FONT_RIGHT 2
#define TEC_FONT_CENTERED 3

#define TAB_STYLE_LEFT 1
#define TAB_STYLE_DOT 2
#define TAB_STYLE_CENTER 3
#define TAB_STYLE_RIGHT 4

typedef struct tagTabs {
    dword Style{};
    dword Width{};
} TABS;

// static_assert(sizeof(TABS) == 8, "TABS size check");

typedef struct tagTextStyle {
    dword Unknown[3]{};
} TEXT_STYLE;

// static_assert(sizeof(TEXT_STYLE) == 12, "TEXT_STYLE size check");

class SB_CFont {
  public:
#pragma pack(push)
#pragma pack(1)
    struct FontHeader {
        word HeaderSize;
        word Unknown0;
        word Flags;
        word Width;
        word Height;
        SLONG Unknown1;
        SLONG BitDepth;
        word NumColors;
        word Unknown2;
        word LoChar;
        word HiChar;
        word Unknown3;
        SLONG szPixels;
        SLONG szColors;
        SLONG Unknown4;
        SLONG szFooter;
    };
#pragma pack(pop)

    SB_CFont(void);
    ~SB_CFont(void);
    SB_CFont(const SB_CFont&) = delete;
    SB_CFont& operator=(const SB_CFont&) = delete;

    void DrawTextA(class SB_CBitmapCore *, SLONG, SLONG, const char *, SLONG = 0, bool = false);
    void DrawTextWithTabs(class SB_CBitmapCore *, SLONG, SLONG, const char *, SLONG = 0, bool = false);
    SLONG DrawTextBlock(class SB_CBitmapCore *, struct tagRECT *, const char *, SLONG = 0, SLONG = 0, bool = false);
    SLONG PreviewTextBlock(class SB_CBitmapCore *, struct tagRECT *, const char *, SLONG = 0, SLONG = 0, bool = false);
    SLONG DrawTextBlock(class SB_CBitmapCore *, SLONG, SLONG, SLONG, SLONG, const char *, SLONG = 0, SLONG = 0, bool = false);
    SLONG PreviewTextBlock(class SB_CBitmapCore *, SLONG, SLONG, SLONG, SLONG, const char *, SLONG = 0, SLONG = 0, bool = false);
    SLONG GetWidthAt(const char *, SLONG, char);
    static SLONG GetWordLength(const char *, SLONG);
    SLONG GetWidth(const char *, SLONG);
    SLONG GetWidth(unsigned char);
    bool Load(SDL_Renderer *, const char *, struct HPALETTE__ * = NULL);
    bool CopyMemToSurface(struct HPALETTE__ *);
    void SetTabulator(struct tagTabs *, ULONG);

    void SetLineSpace(float LineSpace) { this->LineSpace = LineSpace; }

  protected:
    void Init(void);
    void Delete(void);
    bool GetSurface(struct _DDSURFACEDESC *);
    void ReleaseSurface(struct _DDSURFACEDESC *);
    bool DrawChar(unsigned char, bool);
    bool DrawWord(const char *, SLONG);
    unsigned char *GetDataPtr(void);
    bool CreateFontSurface(SDL_Renderer *);
    bool CopyBitmapToMem(struct tagCreateFont *);

  private:
    FontHeader Header{};
    dword Unknown0[4]{};
    SDL_Surface *Surface;
    SDL_Texture *Texture;
    BYTE *VarWidth;
    BYTE *VarHeight;
    bool Hidden;
    TABS *Tabulator;
    word NumTabs{};
    XY Pos;
    XY Start;
    float LineSpace;
    SB_CBitmapCore *Bitmap;
};

// static_assert(sizeof(SB_CFont) == 0x70u, "SB_CFont size check");

