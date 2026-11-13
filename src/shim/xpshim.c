/* XPSHIM.dll - Windows 2000 compatibility functions from windows xp to windows 2000 for the patcher buzzd.chat
 * 
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define XPS_ERR_NOT_IMPL 120

extern void *xps_table[];
extern volatile unsigned int xps_idx;

struct xps_slist_entry  { struct xps_slist_entry *Next; };
struct xps_slist_header { struct xps_slist_entry *Next; WORD Depth; WORD Sequence; };

static CRITICAL_SECTION xps_slist_cs;

/* KERNEL32 fallbacks */

BOOL WINAPI fb_GetLogicalProcessorInformation(LPVOID buf, LPDWORD len)
{
    (void)buf; (void)len;
    SetLastError(XPS_ERR_NOT_IMPL);
    return FALSE;
}

DWORD WINAPI fb_SetThreadExecutionState(DWORD flags)
{
    (void)flags;
    return 0x80000000UL;            /* ES_CONTINUOUS: previous state, shit */
}

BOOL WINAPI fb_GetNumaHighestNodeNumber(PULONG node)
{
    if (node) *node = 0;
    return TRUE;
}

VOID WINAPI fb_InitializeSListHead(struct xps_slist_header *h)
{
    h->Next = NULL; h->Depth = 0; h->Sequence = 0;
}

struct xps_slist_entry * WINAPI fb_InterlockedPushEntrySList(struct xps_slist_header *h,
                                                             struct xps_slist_entry *e)
{
    struct xps_slist_entry *old;
    EnterCriticalSection(&xps_slist_cs);
    old = h->Next;
    e->Next = old;
    h->Next = e;
    h->Depth++;
    h->Sequence++;
    LeaveCriticalSection(&xps_slist_cs);
    return old;
}

struct xps_slist_entry * WINAPI fb_InterlockedPopEntrySList(struct xps_slist_header *h)
{
    struct xps_slist_entry *e;
    EnterCriticalSection(&xps_slist_cs);
    e = h->Next;
    if (e) { h->Next = e->Next; h->Depth--; h->Sequence++; }
    LeaveCriticalSection(&xps_slist_cs);
    return e;
}

struct xps_slist_entry * WINAPI fb_InterlockedFlushSList(struct xps_slist_header *h)
{
    struct xps_slist_entry *e;
    EnterCriticalSection(&xps_slist_cs);
    e = h->Next;
    h->Next = NULL; h->Depth = 0; h->Sequence++;
    LeaveCriticalSection(&xps_slist_cs);
    return e;
}

WORD WINAPI fb_QueryDepthSList(struct xps_slist_header *h)
{
    return h->Depth;
}

typedef struct {
    DWORD     dwLength;
    DWORD     dwMemoryLoad;
    unsigned __int64 ullTotalPhys, ullAvailPhys, ullTotalPageFile, ullAvailPageFile,
                     ullTotalVirtual, ullAvailVirtual, ullAvailExtendedVirtual;
} xps_memstatusex;

BOOL WINAPI fb_GlobalMemoryStatusEx(xps_memstatusex *b)
{
    MEMORYSTATUS ms;
    if (!b || b->dwLength != sizeof(*b)) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);
    b->dwMemoryLoad = ms.dwMemoryLoad;
    b->ullTotalPhys = ms.dwTotalPhys;
    b->ullAvailPhys = ms.dwAvailPhys;
    b->ullTotalPageFile = ms.dwTotalPageFile;
    b->ullAvailPageFile = ms.dwAvailPageFile;
    b->ullTotalVirtual = ms.dwTotalVirtual;
    b->ullAvailVirtual = ms.dwAvailVirtual;
    b->ullAvailExtendedVirtual = 0;
    return TRUE;
}

BOOL WINAPI fb_IsProcessorFeaturePresent(DWORD feature)
{
    (void)feature;
    return FALSE;                   /* conservative: no optional CPU features */
}

PVOID WINAPI fb_EncodePointer(PVOID p) { return p; }
PVOID WINAPI fb_DecodePointer(PVOID p) { return p; }

BOOL WINAPI fb_GetModuleHandleExW(DWORD flags, LPCWSTR name, HMODULE *out)
{
    HMODULE h;
    if (flags & 0x4)                /* GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS */
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((LPCVOID)name, &mbi, sizeof(mbi))) return FALSE;
        h = (HMODULE)mbi.AllocationBase;
    }
    else
        h = GetModuleHandleW(name);
    if (!h) return FALSE;
    if (out) *out = h;
    return TRUE;
}

HANDLE WINAPI fb_FindFirstFileExA(LPCSTR name, DWORD level, LPVOID data, DWORD op,
                                  LPVOID filter, DWORD addflags)
{
    (void)level; (void)op; (void)filter; (void)addflags;
    return FindFirstFileA(name, (LPWIN32_FIND_DATAA)data);
}

HANDLE WINAPI fb_FindFirstFileExW(LPCWSTR name, DWORD level, LPVOID data, DWORD op,
                                  LPVOID filter, DWORD addflags)
{
    (void)level; (void)op; (void)filter; (void)addflags;
    return FindFirstFileW(name, (LPWIN32_FIND_DATAW)data);
}

BOOL WINAPI fb_GetFileSizeEx(HANDLE h, LARGE_INTEGER *size)
{
    DWORD lo, hi = 0;
    lo = GetFileSize(h, &hi);
    if (lo == 0xFFFFFFFF && GetLastError() != NO_ERROR) return FALSE;
    size->LowPart = lo;
    size->HighPart = (LONG)hi;
    return TRUE;
}

BOOL WINAPI fb_InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION cs, DWORD spin)
{
    (void)spin;
    InitializeCriticalSection(cs);
    return TRUE;
}

/* USER32 fallbacks (Raw Input does not exist on this w2000 need more work) */

BOOL WINAPI fb_RegisterRawInputDevices(LPVOID devs, UINT n, UINT size)
{
    (void)devs; (void)n; (void)size;
    SetLastError(XPS_ERR_NOT_IMPL);
    return FALSE;
}

UINT WINAPI fb_GetRawInputData(HANDLE h, UINT cmd, LPVOID data, PUINT size, UINT hsz)
{
    (void)h; (void)cmd; (void)data; (void)size; (void)hsz;
    SetLastError(XPS_ERR_NOT_IMPL);
    return (UINT)-1;
}

UINT WINAPI fb_GetRawInputDeviceList(LPVOID list, PUINT n, UINT size)
{
    (void)list; (void)size;
    if (n) *n = 0;
    return 0;
}

UINT WINAPI fb_GetRawInputDeviceInfoA(HANDLE h, UINT cmd, LPVOID data, PUINT size)
{
    (void)h; (void)cmd; (void)data; (void)size;
    SetLastError(XPS_ERR_NOT_IMPL);
    return (UINT)-1;
}

/*SHELL32 fallback */

typedef BOOL (WINAPI *xps_sgsfp_t)(HWND, LPWSTR, int, BOOL);
static xps_sgsfp_t xps_sgsfp;

HRESULT WINAPI fb_SHGetFolderPathW(HWND hwnd, int csidl, HANDLE token, DWORD flags, LPWSTR path)
{
    (void)token; (void)flags;
    if (!xps_sgsfp)
    {
        HMODULE m = LoadLibraryA("SHELL32.dll");
        if (m) xps_sgsfp = (xps_sgsfp_t)GetProcAddress(m, "SHGetSpecialFolderPathW");
        if (!xps_sgsfp) return E_FAIL;
    }
    if (!path) return E_INVALIDARG;
    return xps_sgsfp(hwnd, path, csidl & 0xFF, (csidl & 0x8000) != 0) ? S_OK : E_FAIL;
}

/* XP version spoof
 * (probably is not needed to work but is here for next versions :]]]] )
 */
#define XPS_SPOOF_MAJOR   5
#define XPS_SPOOF_MINOR   1
#define XPS_SPOOF_BUILD   2600
#define XPS_SPOOF_PLATFORM 2      /* VER_PLATFORM_WIN32_NT */
#define XPS_SPOOF_SPMAJ   2
#define XPS_SPOOF_SPMIN   0

static void xps_fill_common(OSVERSIONINFOW *v)
{
    v->dwMajorVersion = XPS_SPOOF_MAJOR;
    v->dwMinorVersion = XPS_SPOOF_MINOR;
    v->dwBuildNumber  = XPS_SPOOF_BUILD;
    v->dwPlatformId   = XPS_SPOOF_PLATFORM;
}

BOOL WINAPI xps_GetVersionExW(OSVERSIONINFOW *v)
{
    if (!v) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW) &&
        v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)) {
        SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
    }
    xps_fill_common(v);
    {
        static const WCHAR csd[] = L"Service Pack 2";
        int i;
        for (i = 0; csd[i]; i++) v->szCSDVersion[i] = csd[i];
        v->szCSDVersion[i] = 0;
    }
    if (v->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW)) {
        OSVERSIONINFOEXW *ex = (OSVERSIONINFOEXW *)v;
        ex->wServicePackMajor = XPS_SPOOF_SPMAJ;
        ex->wServicePackMinor = XPS_SPOOF_SPMIN;
        ex->wSuiteMask = VER_SUITE_SINGLEUSERTS;
        ex->wProductType = VER_NT_WORKSTATION;
        ex->wReserved = 0;
    }
    return TRUE;
}

BOOL WINAPI xps_GetVersionExA(OSVERSIONINFOA *v)
{
    OSVERSIONINFOEXW w;
    if (!v) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA) &&
        v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA)) {
        SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
    }
    w.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    xps_GetVersionExW((OSVERSIONINFOW *)&w);
    v->dwMajorVersion = w.dwMajorVersion;
    v->dwMinorVersion = w.dwMinorVersion;
    v->dwBuildNumber  = w.dwBuildNumber;
    v->dwPlatformId   = w.dwPlatformId;
    WideCharToMultiByte(CP_ACP, 0, w.szCSDVersion, -1, v->szCSDVersion,
                        sizeof(v->szCSDVersion), NULL, NULL);
    if (v->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA)) {
        OSVERSIONINFOEXA *ex = (OSVERSIONINFOEXA *)v;
        ex->wServicePackMajor = w.wServicePackMajor;
        ex->wServicePackMinor = w.wServicePackMinor;
        ex->wSuiteMask = w.wSuiteMask;
        ex->wProductType = w.wProductType;
        ex->wReserved = 0;
    }
    return TRUE;
}

/* Condition bits and operator codes MSDN */
#define XPS_VER_MINORVERSION     0x0000001
#define XPS_VER_MAJORVERSION     0x0000002
#define XPS_VER_BUILDNUMBER      0x0000004
#define XPS_VER_PLATFORMID       0x0000008
#define XPS_VER_SERVICEPACKMINOR 0x0000010
#define XPS_VER_SERVICEPACKMAJOR 0x0000020

static int xps_cmp(DWORD cur, DWORD req, int op)
{
    switch (op) {
    case 1: return cur == req;      /* VER_EQUAL */
    case 2: return cur >  req;      /* VER_GREATER */
    case 3: return cur >= req;      /* VER_GREATER_EQUAL */
    case 4: return cur <  req;      /* VER_LESS */
    case 5: return cur <= req;      /* VER_LESS_EQUAL */
    }
    return 0;
}

static int xps_check(DWORD typebit, DWORD cur, DWORD req, unsigned __int64 cond)
{
    int idx = 0, op;
    while ((1UL << idx) < typebit) idx++;
    op = (int)((cond >> (idx * 3)) & 7);
    return xps_cmp(cur, req, op);
}

BOOL WINAPI xps_VerifyVersionInfoW(OSVERSIONINFOEXW *req, DWORD typemask, unsigned __int64 cond)
{
    if (!req) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if ((typemask & XPS_VER_MAJORVERSION) && !xps_check(XPS_VER_MAJORVERSION, XPS_SPOOF_MAJOR, req->dwMajorVersion, cond)) goto fail;
    if ((typemask & XPS_VER_MINORVERSION) && !xps_check(XPS_VER_MINORVERSION, XPS_SPOOF_MINOR, req->dwMinorVersion, cond)) goto fail;
    if ((typemask & XPS_VER_BUILDNUMBER)  && !xps_check(XPS_VER_BUILDNUMBER,  XPS_SPOOF_BUILD, req->dwBuildNumber, cond)) goto fail;
    if ((typemask & XPS_VER_PLATFORMID)   && !xps_check(XPS_VER_PLATFORMID,   XPS_SPOOF_PLATFORM, req->dwPlatformId, cond)) goto fail;
    if ((typemask & XPS_VER_SERVICEPACKMAJOR) && !xps_check(XPS_VER_SERVICEPACKMAJOR, XPS_SPOOF_SPMAJ, req->wServicePackMajor, cond)) goto fail;
    if ((typemask & XPS_VER_SERVICEPACKMINOR) && !xps_check(XPS_VER_SERVICEPACKMINOR, XPS_SPOOF_SPMIN, req->wServicePackMinor, cond)) goto fail;
    return TRUE;
fail:
    SetLastError(1150);             /* ERROR_OLD_WIN_VERSION */
    return FALSE;
}

/* A replacement for name, or NULL to keep the system function. */
static void *xps_override(const char *name)
{
    if (lstrcmpA(name, "GetVersionExW") == 0) return (void *)xps_GetVersionExW;
    if (lstrcmpA(name, "GetVersionExA") == 0) return (void *)xps_GetVersionExA;
    if (lstrcmpA(name, "VerifyVersionInfoW") == 0) return (void *)xps_VerifyVersionInfoW;
    return NULL;
}

/* lazyyyyyyyyyyyy resolution */

#include "xpshim_gen.h"

extern volatile unsigned int xps_idx;
extern void *xps_table[];

static HMODULE xps_mods[XPS_NLIB];

static void xps_unresolved(void)
{
    static char msg[300];
    unsigned int i = xps_idx < XPS_NUM ? xps_idx : 0;
    lstrcpyA(msg, "XPSH: Upsss... is missing a function: "); /* again for next versions to debug*/
    lstrcatA(msg, xps_names[i]);
    MessageBoxA(NULL, msg, "XPSH", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

/* Asm trampoline (_xps_lazy) on first use of an entry.*/
void *xps_resolve(void)
{
    unsigned int i = xps_idx;
    FARPROC p = NULL;
    int li;
    if (i >= XPS_NUM) return (void *)xps_unresolved;
    li = xps_lib[i];
    if (!xps_mods[li])
        xps_mods[li] = LoadLibraryA(xps_libname[li]);
    if (xps_mods[li])
        p = GetProcAddress(xps_mods[li], xps_names[i]);
    if (!p)
        p = (FARPROC)xps_fb[i];
    {
        void *ov = xps_override(xps_names[i]);
        if (ov) p = (FARPROC)ov;
    }
    if (!p) p = (FARPROC)xps_unresolved;
    xps_table[i] = (void *)p;
    return (void *)p;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(inst);
        InitializeCriticalSection(&xps_slist_cs);
    }
    return TRUE;
}
