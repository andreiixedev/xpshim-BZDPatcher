/* BZDPatcher setup for Windows 2000.
 * Creates a patched copy with the required compatibility files.
 * The original Buzzd Patcher is left unchanged.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <objbase.h>

#define DST_DIR    "BZDPatcher_Win2000"
#define DST_NAME   "BZDPatcher.exe"
#define SRC_SIZE   3639808UL
#define SHIM_NAME  "XPSH.dll"
#define HELPER_NAME "reg.exe"
#define RES_SHIM   101
#define RES_HELPER 102

typedef struct { DWORD off; const char *name; } ImportName;

/* Import DLL names (file offsets). */
static const ImportName IMPORTS[] = {
    {0x298e92, "VERSION.dll"},  {0x298e9e, "IMM32.dll"},   {0x298ea8, "WINMM.dll"},
    {0x298eb2, "SETUPAPI.dll"}, {0x2997ca, "KERNEL32.dll"},{0x29a02c, "USER32.dll"},
    {0x29a1f6, "GDI32.dll"},    {0x29a214, "COMDLG32.dll"},{0x29a254, "ADVAPI32.dll"},
    {0x29a2c8, "SHELL32.dll"},  {0x29a342, "ole32.dll"},   {0x29a34c, "OLEAUT32.dll"},
    {0x29a372, "PSAPI.DLL"}
};
#define NIMPORTS (sizeof(IMPORTS) / sizeof(IMPORTS[0]))

/* Original minor OS and subsystem versions, HM */
#define OFF_MINOR_OS      0x17a
#define OFF_MINOR_SUBSYS  0x182

static char g_msg[1024];

static void fail(const char *text)
{
    MessageBoxA(NULL, text, "BZDPatcher Win2000 setup", MB_ICONERROR | MB_OK);
}

static BOOL write_file(const char *path, const void *data, DWORD size)
{
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    BOOL ok;
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;
    ok = WriteFile(h, data, size, &written, NULL) && written == size;
    CloseHandle(h);
    return ok;
}

static BOOL extract_resource(const char *dir, int id, const char *name)
{
    HRSRC r = FindResourceA(NULL, MAKEINTRESOURCEA(id), RT_RCDATA);
    HGLOBAL g;
    void *p;
    DWORD size;
    char path[MAX_PATH * 2];
    if (!r) return FALSE;
    size = SizeofResource(NULL, r);
    g = LoadResource(NULL, r);
    if (!g) return FALSE;
    p = LockResource(g);
    if (!p) return FALSE;
    lstrcpyA(path, dir);
    lstrcatA(path, "\\");
    lstrcatA(path, name);
    return write_file(path, p, size);
}

static BOOL pick_source(char *out, DWORD n)
{
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "BZDPatcher (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = out;
    ofn.nMaxFile = n;
    ofn.lpstrTitle = "Select the original BZDPatcher_1_2_2.exe";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    out[0] = 0;
    return GetOpenFileNameA(&ofn);
}

static BOOL pick_folder(char *out)
{
    BROWSEINFOA bi;
    LPITEMIDLIST pidl;
    BOOL ok = FALSE;
    ZeroMemory(&bi, sizeof(bi));
    bi.lpszTitle = "Choose the folder where BZDPatcher_Win2000 should be created";
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    pidl = SHBrowseForFolderA(&bi);
    if (!pidl) return FALSE;
    ok = SHGetPathFromIDListA(pidl, out);
    CoTaskMemFree(pidl);
    return ok;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
    char src[MAX_PATH * 2], folder[MAX_PATH * 2], outdir[MAX_PATH * 2], dst[MAX_PATH * 2];
    unsigned char *buf = NULL;
    DWORD size = 0, got = 0, i;
    HANDLE h;
    (void)inst; (void)prev; (void)cmdline; (void)show;

    CoInitialize(NULL);

    /* Step 1. pick the fuking file */
    if (!pick_source(src, sizeof(src))) { CoUninitialize(); return 0; }

    h = CreateFileA(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { fail("Cannot open the selected file."); goto bad_nobuf; }
    size = GetFileSize(h, NULL);
    if (size != SRC_SIZE) {
        CloseHandle(h);
        wsprintfA(g_msg, "Wrong file size (%lu bytes). Select the original BZDPatcher_1_2_2.exe "
                         "(%lu bytes).", (unsigned long)size, (unsigned long)SRC_SIZE);
        fail(g_msg);
        goto bad_nobuf;
    }
    buf = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    if (!buf || !ReadFile(h, buf, size, &got, NULL) || got != size) {
        CloseHandle(h);
        fail("Cannot read the selected file.");
        goto bad;
    }
    CloseHandle(h);

    /* verify every byte, pls to work for next versions */
    if (buf[0] != 'M' || buf[1] != 'Z') { fail("Not a Windows executable."); goto bad; }
    if (buf[OFF_MINOR_OS] != 1 || buf[OFF_MINOR_SUBSYS] != 1) {
        fail("Unexpected version fields; this is not the original BZDPatcher 1.2.2."); goto bad;
    }
    for (i = 0; i < NIMPORTS; i++) {
        DWORD len = lstrlenA(IMPORTS[i].name) + 1;
        if (IMPORTS[i].off + len > size || memcmp(buf + IMPORTS[i].off, IMPORTS[i].name, len) != 0) {
            wsprintfA(g_msg, "Unexpected import name at 0x%lx (%s). Not the original file.",
                      (unsigned long)IMPORTS[i].off, IMPORTS[i].name);
            fail(g_msg);
            goto bad;
        }
    }

    /* Step 2 (send the message?) hell no, where to put the patcher patched for w2000 */
    if (!pick_folder(folder)) { goto bad; }
    lstrcpyA(outdir, folder);
    lstrcatA(outdir, "\\");
    lstrcatA(outdir, DST_DIR);
    if (!CreateDirectoryA(outdir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        wsprintfA(g_msg, "Cannot create the folder:\n%s", outdir);
        fail(g_msg);
        goto bad;
    }

    /* Step3 idk hope to work but it's works */
    buf[OFF_MINOR_OS] = 0;
    buf[OFF_MINOR_SUBSYS] = 0;
    for (i = 0; i < NIMPORTS; i++) {
        DWORD len = lstrlenA(IMPORTS[i].name) + 1;
        memset(buf + IMPORTS[i].off, 0, len);
        memcpy(buf + IMPORTS[i].off, SHIM_NAME, lstrlenA(SHIM_NAME) + 1);
    }

    lstrcpyA(dst, outdir);
    lstrcatA(dst, "\\");
    lstrcatA(dst, DST_NAME);
    if (lstrcmpiA(src, dst) == 0) { fail("Output would overwrite the original file."); goto bad; }
    if (!write_file(dst, buf, size)) {
        wsprintfA(g_msg, "Cannot write:\n%s", dst);
        fail(g_msg);
        goto bad;
    }
    HeapFree(GetProcessHeap(), 0, buf);
    buf = NULL;

    if (!extract_resource(outdir, RES_SHIM, SHIM_NAME) ||
        !extract_resource(outdir, RES_HELPER, HELPER_NAME)) {
        fail("BZDPatcher.exe was written, but XPSH.dll or reg.exe could not be written.");
        CoUninitialize();
        return 1;
    }

    wsprintfA(g_msg, "Done.\n\nFolder: %s\n\nCreated: %s, %s, %s\n\n"
                     "Run BZDPatcher.exe from this folder.", outdir, DST_NAME, SHIM_NAME, HELPER_NAME);
    MessageBoxA(NULL, g_msg, "BZDPatcher Win2000 setup", MB_ICONINFORMATION | MB_OK);
    CoUninitialize();
    return 0;

bad:
    if (buf) HeapFree(GetProcessHeap(), 0, buf);
bad_nobuf:
    CoUninitialize();
    return 1;
}
