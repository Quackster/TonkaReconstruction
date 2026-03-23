/*
 * WING32.DLL compatibility shim for Tonka Construction
 *
 * 1) Implements the WinG 1.0 API using modern GDI equivalents.
 * 2) On load, IAT-hooks GetPrivateProfileStringA in TONKA.EXE so that
 *    "Tonka.ini" resolves relative to the EXE directory instead of
 *    the Windows directory (the Win32 default for bare filenames).
 * 3) Auto-creates Tonka.ini + saves directory if missing.
 * 4) Hooks CreateFileA to ensure game files are found even with
 *    long working directory paths (prevents buffer overflows in the
 *    game's internal error-message sprintf).
 * 5) Hooks SetWindowPos to replace HWND_TOPMOST with HWND_TOP,
 *    making the game behave as borderless windowed (alt-tab friendly).
 * 6) Hooks _getcwd (Borland CRT) so the game's resource manager
 *    stores a short base path, avoiding overflow of its fixed-size
 *    error buffer at 0x004328bc into the resource-manager pointer
 *    at 0x0043290c.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Debug logging                                                     */
/* ------------------------------------------------------------------ */

static FILE *g_logFile;

static void LogInit(void)
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char *s = strrchr(path, '\\');
    if (s) s[1] = '\0';
    strcat(path, "wing32_debug.log");
    g_logFile = fopen(path, "w");
}

static void Log(const char *fmt, ...)
{
    if (!g_logFile) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_logFile, fmt, ap);
    va_end(ap);
    fflush(g_logFile);
}

/* ------------------------------------------------------------------ */
/*  INI path fix — IAT hook for GetPrivateProfileStringA              */
/* ------------------------------------------------------------------ */

static char g_iniFullPath[MAX_PATH];
static char g_exeDir[MAX_PATH];     /* EXE directory with trailing \ */
static int  g_exeDirLen;

typedef DWORD (WINAPI *PFN_GetPrivateProfileStringA)(
    LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);

static PFN_GetPrivateProfileStringA g_origGetPrivateProfileStringA;

static DWORD WINAPI Hooked_GetPrivateProfileStringA(
    LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault,
    LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
    if (lpFileName && _stricmp(lpFileName, "Tonka.ini") == 0) {
        Log("GetPrivateProfileStringA: redirecting '%s' -> '%s'\n",
            lpFileName, g_iniFullPath);
        lpFileName = g_iniFullPath;
    }
    DWORD result = g_origGetPrivateProfileStringA(
        lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
    Log("  result=%lu, returned='%s'\n", result,
        lpReturnedString ? lpReturnedString : "(null)");
    return result;
}

typedef DWORD (WINAPI *PFN_WritePrivateProfileStringA)(
    LPCSTR, LPCSTR, LPCSTR, LPCSTR);

static PFN_WritePrivateProfileStringA g_origWritePrivateProfileStringA;

static DWORD WINAPI Hooked_WritePrivateProfileStringA(
    LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName)
{
    if (lpFileName && _stricmp(lpFileName, "Tonka.ini") == 0) {
        Log("WritePrivateProfileStringA: redirecting '%s' -> '%s'\n",
            lpFileName, g_iniFullPath);
        lpFileName = g_iniFullPath;
    }
    return g_origWritePrivateProfileStringA(
        lpAppName, lpKeyName, lpString, lpFileName);
}

/* ------------------------------------------------------------------ */
/*  CreateFileA hook — resolve relative paths against EXE directory    */
/* ------------------------------------------------------------------ */

typedef HANDLE (WINAPI *PFN_CreateFileA)(
    LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

static PFN_CreateFileA g_origCreateFileA;

static HANDLE WINAPI Hooked_CreateFileA(
    LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE h = g_origCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);

    /* If the open failed and the path looks relative (no : or leading \),
       retry with the EXE directory prepended.  This handles the case where
       the game's CWD differs from the EXE directory. */
    if (h == INVALID_HANDLE_VALUE && lpFileName &&
        lpFileName[0] != '\\' && lpFileName[0] != '/' &&
        (strlen(lpFileName) < 2 || lpFileName[1] != ':'))
    {
        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s%s", g_exeDir, lpFileName);
        Log("CreateFileA: retry '%s' -> '%s'\n", lpFileName, fullPath);
        h = g_origCreateFileA(fullPath, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
            hTemplateFile);
    }

    if (h == INVALID_HANDLE_VALUE) {
        Log("CreateFileA FAILED: '%s' (err=%lu)\n", lpFileName, GetLastError());
    }

    return h;
}

/* ------------------------------------------------------------------ */
/*  _getcwd hook — return a short path to prevent buffer overflows    */
/*                                                                    */
/*  The game's resource manager (FUN_0042ad91) calls _getcwd to get   */
/*  a base directory, then stores it in a fixed-size buffer.  Later,  */
/*  error messages are sprintf'd into an 80-byte gap before the       */
/*  resource-manager pointer.  Long modern paths overflow this gap.   */
/*  Returning the 8.3 short path (or ".\") keeps things safe.        */
/* ------------------------------------------------------------------ */

typedef char* (__cdecl *PFN_getcwd)(char *, int);

static PFN_getcwd g_orig_getcwd;

static char* __cdecl Hooked_getcwd(char *buf, int maxlen)
{
    /* Get the short (8.3) form of the EXE directory.
       This keeps the game's internal paths short enough to avoid
       overflowing its fixed-size error-message buffers. */
    char shortPath[MAX_PATH];
    DWORD len = GetShortPathNameA(g_exeDir, shortPath, MAX_PATH);

    if (len > 0 && len < (DWORD)maxlen) {
        /* Remove trailing backslash — _getcwd doesn't include one
           (unless it's a drive root like "C:\") */
        int slen = (int)strlen(shortPath);
        if (slen > 3 && shortPath[slen - 1] == '\\')
            shortPath[slen - 1] = '\0';
        strncpy(buf, shortPath, maxlen);
        buf[maxlen - 1] = '\0';
        Log("_getcwd hooked: returning '%s' (was '%s')\n", buf, g_exeDir);
        return buf;
    }

    /* Fallback: use the original _getcwd */
    char *result = g_orig_getcwd(buf, maxlen);
    Log("_getcwd fallback: '%s'\n", result ? result : "(null)");
    return result;
}

/* ------------------------------------------------------------------ */
/*  SetWindowPos hook — prevent HWND_TOPMOST for borderless windowed  */
/*                                                                    */
/*  The game sets its main window to HWND_TOPMOST, which prevents     */
/*  alt-tab from bringing other windows in front.  Replacing it with  */
/*  HWND_TOP keeps the same z-order behaviour on creation but allows  */
/*  normal window switching.                                          */
/* ------------------------------------------------------------------ */

typedef BOOL (WINAPI *PFN_SetWindowPos)(
    HWND, HWND, int, int, int, int, UINT);

static PFN_SetWindowPos g_origSetWindowPos;

static BOOL WINAPI Hooked_SetWindowPos(
    HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    if (hWndInsertAfter == HWND_TOPMOST) {
        Log("SetWindowPos: replacing HWND_TOPMOST with HWND_TOP\n");
        hWndInsertAfter = HWND_TOP;
    }
    return g_origSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

/* ------------------------------------------------------------------ */
/*  IAT patching                                                      */
/* ------------------------------------------------------------------ */

static void PatchIATEntry(const char *dllName, const char *funcName,
                          FARPROC newFunc, FARPROC *pOrig)
{
    HMODULE hExe = GetModuleHandleA(NULL);
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)hExe;
    PIMAGE_NT_HEADERS pNT = (PIMAGE_NT_HEADERS)((BYTE*)hExe + pDos->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR pImport = (PIMAGE_IMPORT_DESCRIPTOR)(
        (BYTE*)hExe + pNT->OptionalHeader.DataDirectory[1].VirtualAddress);

    FARPROC pfnTarget = GetProcAddress(
        GetModuleHandleA(dllName), funcName);

    if (!pfnTarget) {
        Log("IAT hook: %s!%s not found via GetProcAddress\n", dllName, funcName);
        *pOrig = NULL;
        return;
    }

    for (; pImport->Name; pImport++) {
        PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(
            (BYTE*)hExe + pImport->FirstThunk);
        for (; pThunk->u1.Function; pThunk++) {
            if ((FARPROC)pThunk->u1.Function == pfnTarget) {
                DWORD oldProt;
                VirtualProtect(&pThunk->u1.Function, sizeof(FARPROC),
                               PAGE_READWRITE, &oldProt);
                *pOrig = (FARPROC)pThunk->u1.Function;
                pThunk->u1.Function = (ULONG_PTR)newFunc;
                VirtualProtect(&pThunk->u1.Function, sizeof(FARPROC),
                               oldProt, &oldProt);
                Log("IAT hook: %s!%s patched OK\n", dllName, funcName);
                return;
            }
        }
    }
    *pOrig = pfnTarget;
    Log("IAT hook: %s!%s not found in IAT, using direct\n", dllName, funcName);
}

static void PatchIAT(void)
{
    PatchIATEntry("KERNEL32.dll", "GetPrivateProfileStringA",
        (FARPROC)Hooked_GetPrivateProfileStringA,
        (FARPROC*)&g_origGetPrivateProfileStringA);
    PatchIATEntry("KERNEL32.dll", "WritePrivateProfileStringA",
        (FARPROC)Hooked_WritePrivateProfileStringA,
        (FARPROC*)&g_origWritePrivateProfileStringA);
    PatchIATEntry("KERNEL32.dll", "CreateFileA",
        (FARPROC)Hooked_CreateFileA,
        (FARPROC*)&g_origCreateFileA);
    PatchIATEntry("USER32.dll", "SetWindowPos",
        (FARPROC)Hooked_SetWindowPos,
        (FARPROC*)&g_origSetWindowPos);
    PatchIATEntry("CW3215.DLL", "_getcwd",
        (FARPROC)Hooked_getcwd,
        (FARPROC*)&g_orig_getcwd);
}

static void EnsureIniExists(void)
{
    GetModuleFileNameA(NULL, g_exeDir, MAX_PATH);
    char *lastSlash = strrchr(g_exeDir, '\\');
    if (lastSlash) lastSlash[1] = '\0';
    g_exeDirLen = (int)strlen(g_exeDir);

    snprintf(g_iniFullPath, MAX_PATH, "%sTonka.ini", g_exeDir);
    Log("EXE dir: %s\n", g_exeDir);
    Log("INI path: %s\n", g_iniFullPath);

    if (GetFileAttributesA(g_iniFullPath) == INVALID_FILE_ATTRIBUTES) {
        char savesDir[MAX_PATH];
        snprintf(savesDir, MAX_PATH, "%ssaves", g_exeDir);
        CreateDirectoryA(savesDir, NULL);

        HANDLE hFile = CreateFileA(g_iniFullPath, GENERIC_WRITE, 0, NULL,
                                   CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            /* Use a short relative path for SaveDir to avoid overflowing
               the game's internal 128-byte path buffers */
            char buf[512];
            int len = snprintf(buf, sizeof(buf),
                "[Directories]\r\nSaveDir=.\\saves\r\n");
            DWORD written;
            WriteFile(hFile, buf, len, &written, NULL);
            CloseHandle(hFile);
            Log("Created Tonka.ini with SaveDir=.\\saves\n");
        }
    }
}

/* ------------------------------------------------------------------ */
/*  WinG API — fully implemented (used by TONKA.EXE)                  */
/* ------------------------------------------------------------------ */

__declspec(dllexport) HDC WINAPI WinGCreateDC(void)
{
    HDC hdc = CreateCompatibleDC(NULL);
    Log("WinGCreateDC() = %p\n", hdc);
    return hdc;
}

__declspec(dllexport) HBITMAP WINAPI WinGCreateBitmap(HDC hdc,
    const BITMAPINFO *pHeader, void **ppBits)
{
    Log("WinGCreateBitmap(hdc=%p, w=%d, h=%d, bpp=%d)\n",
        hdc,
        pHeader ? pHeader->bmiHeader.biWidth : -1,
        pHeader ? pHeader->bmiHeader.biHeight : -1,
        pHeader ? pHeader->bmiHeader.biBitCount : -1);
    HBITMAP hbm = CreateDIBSection(hdc, pHeader, DIB_RGB_COLORS, ppBits, NULL, 0);
    Log("  => hbm=%p, bits=%p\n", hbm, ppBits ? *ppBits : NULL);
    return hbm;
}

__declspec(dllexport) BOOL WINAPI WinGBitBlt(HDC hdcDst, int xDst, int yDst,
    int nWidth, int nHeight, HDC hdcSrc, int xSrc, int ySrc)
{
    Log("WinGBitBlt(dst=%p, %d,%d,%d,%d, src=%p, %d,%d)\n",
        hdcDst, xDst, yDst, nWidth, nHeight, hdcSrc, xSrc, ySrc);
    return BitBlt(hdcDst, xDst, yDst, nWidth, nHeight,
                  hdcSrc, xSrc, ySrc, SRCCOPY);
}

__declspec(dllexport) UINT WINAPI WinGSetDIBColorTable(HDC hdc,
    UINT iStart, UINT cEntries, const RGBQUAD *pColors)
{
    Log("WinGSetDIBColorTable(hdc=%p, start=%u, count=%u)\n",
        hdc, iStart, cEntries);
    return SetDIBColorTable(hdc, iStart, cEntries, pColors);
}

/* ------------------------------------------------------------------ */
/*  WinG API — stubs (not used by TONKA.EXE)                          */
/* ------------------------------------------------------------------ */

__declspec(dllexport) BOOL WINAPI WinGRecommendDIBFormat(BITMAPINFO *pFormat)
{
    if (!pFormat) return FALSE;
    HDC hdc = GetDC(NULL);
    BITMAPINFOHEADER *h = &pFormat->bmiHeader;
    h->biSize = sizeof(BITMAPINFOHEADER);
    h->biWidth = GetDeviceCaps(hdc, HORZRES);
    h->biHeight = -GetDeviceCaps(hdc, VERTRES);
    h->biPlanes = 1;
    h->biBitCount = 8;
    h->biCompression = BI_RGB;
    h->biSizeImage = 0;
    h->biXPelsPerMeter = 0;
    h->biYPelsPerMeter = 0;
    h->biClrUsed = 0;
    h->biClrImportant = 0;
    ReleaseDC(NULL, hdc);
    return TRUE;
}

__declspec(dllexport) void* WINAPI WinGGetDIBPointer(HBITMAP hBitmap,
    BITMAPINFO *pHeader)
{
    DIBSECTION ds;
    if (GetObject(hBitmap, sizeof(ds), &ds)) {
        if (pHeader) pHeader->bmiHeader = ds.dsBmih;
        return ds.dsBm.bmBits;
    }
    return NULL;
}

__declspec(dllexport) UINT WINAPI WinGGetDIBColorTable(HDC hdc,
    UINT iStart, UINT cEntries, RGBQUAD *pColors)
{
    return GetDIBColorTable(hdc, iStart, cEntries, pColors);
}

__declspec(dllexport) HPALETTE WINAPI WinGCreateHalftonePalette(void)
{
    HDC hdc = GetDC(NULL);
    HPALETTE hpal = CreateHalftonePalette(hdc);
    ReleaseDC(NULL, hdc);
    return hpal;
}

__declspec(dllexport) HBRUSH WINAPI WinGCreateHalftoneBrush(HDC hdc,
    COLORREF crColor, int nDitherType)
{
    return CreateSolidBrush(crColor);
}

__declspec(dllexport) BOOL WINAPI WinGStretchBlt(HDC hdcDst, int xDst,
    int yDst, int nWidthDst, int nHeightDst, HDC hdcSrc, int xSrc,
    int ySrc, int nWidthSrc, int nHeightSrc)
{
    SetStretchBltMode(hdcDst, COLORONCOLOR);
    return StretchBlt(hdcDst, xDst, yDst, nWidthDst, nHeightDst,
                      hdcSrc, xSrc, ySrc, nWidthSrc, nHeightSrc, SRCCOPY);
}

/* ------------------------------------------------------------------ */
/*  Crash handler — logs exception info before dying                  */
/* ------------------------------------------------------------------ */

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS *ep)
{
    Log("\n!!! CRASH !!!\n");
    Log("Exception code: 0x%08X\n", ep->ExceptionRecord->ExceptionCode);
    Log("Exception addr: 0x%08X\n", (unsigned)ep->ExceptionRecord->ExceptionAddress);
    Log("EIP: 0x%08X\n", ep->ContextRecord->Eip);
    Log("EAX: 0x%08X  EBX: 0x%08X\n", ep->ContextRecord->Eax, ep->ContextRecord->Ebx);
    Log("ECX: 0x%08X  EDX: 0x%08X\n", ep->ContextRecord->Ecx, ep->ContextRecord->Edx);
    Log("ESI: 0x%08X  EDI: 0x%08X\n", ep->ContextRecord->Esi, ep->ContextRecord->Edi);
    Log("ESP: 0x%08X  EBP: 0x%08X\n", ep->ContextRecord->Esp, ep->ContextRecord->Ebp);
    if (ep->ExceptionRecord->ExceptionCode == 0xC0000005) {
        Log("Access violation %s address 0x%08X\n",
            ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
            (unsigned)ep->ExceptionRecord->ExceptionInformation[1]);
    }
    /* Dump the resource-manager pointer for diagnosis */
    Log("DAT_0043290c = 0x%08X\n", *(unsigned *)0x0043290c);
    if (g_logFile) fclose(g_logFile);
    g_logFile = NULL;
    return EXCEPTION_CONTINUE_SEARCH;
}

/* ------------------------------------------------------------------ */
/*  DLL entry point                                                   */
/* ------------------------------------------------------------------ */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        LogInit();
        Log("=== WING32 shim loaded ===\n");
        SetUnhandledExceptionFilter(CrashHandler);
        EnsureIniExists();
        PatchIAT();
        Log("=== Init complete ===\n");
    }
    if (fdwReason == DLL_PROCESS_DETACH) {
        Log("=== WING32 shim unloading ===\n");
        if (g_logFile) fclose(g_logFile);
    }
    return TRUE;
}
