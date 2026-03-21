/*
 * WING32.DLL compatibility shim for Tonka Construction
 *
 * 1) Implements the WinG 1.0 API using modern GDI equivalents.
 * 2) On load, IAT-hooks GetPrivateProfileStringA in TONKA.EXE so that
 *    "Tonka.ini" resolves relative to the EXE directory instead of
 *    the Windows directory (the Win32 default for bare filenames).
 * 3) Auto-creates Tonka.ini + saves directory if missing.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  INI path fix — IAT hook for GetPrivateProfileStringA              */
/* ------------------------------------------------------------------ */

static char g_iniFullPath[MAX_PATH];   /* "C:\...\DATA\Tonka.ini" */
static char g_exeDir[MAX_PATH];        /* "C:\...\DATA\"           */

typedef DWORD (WINAPI *PFN_GetPrivateProfileStringA)(
    LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);

static PFN_GetPrivateProfileStringA g_origGetPrivateProfileStringA;

static DWORD WINAPI Hooked_GetPrivateProfileStringA(
    LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault,
    LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
    /* Redirect bare "Tonka.ini" to the full path next to the EXE */
    if (lpFileName && _stricmp(lpFileName, "Tonka.ini") == 0) {
        lpFileName = g_iniFullPath;
    }
    return g_origGetPrivateProfileStringA(
        lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
}

static void PatchIAT(void)
{
    HMODULE hExe = GetModuleHandleA(NULL);
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)hExe;
    PIMAGE_NT_HEADERS pNT = (PIMAGE_NT_HEADERS)((BYTE*)hExe + pDos->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR pImport = (PIMAGE_IMPORT_DESCRIPTOR)(
        (BYTE*)hExe + pNT->OptionalHeader.DataDirectory[1].VirtualAddress);

    FARPROC pfnTarget = GetProcAddress(
        GetModuleHandleA("KERNEL32.dll"), "GetPrivateProfileStringA");

    for (; pImport->Name; pImport++) {
        PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(
            (BYTE*)hExe + pImport->FirstThunk);
        for (; pThunk->u1.Function; pThunk++) {
            if ((FARPROC)pThunk->u1.Function == pfnTarget) {
                DWORD oldProt;
                VirtualProtect(&pThunk->u1.Function, sizeof(FARPROC),
                               PAGE_READWRITE, &oldProt);
                g_origGetPrivateProfileStringA =
                    (PFN_GetPrivateProfileStringA)pThunk->u1.Function;
                pThunk->u1.Function = (ULONG_PTR)Hooked_GetPrivateProfileStringA;
                VirtualProtect(&pThunk->u1.Function, sizeof(FARPROC),
                               oldProt, &oldProt);
                return;
            }
        }
    }
    /* Fallback if IAT entry not found — use direct pointer */
    g_origGetPrivateProfileStringA =
        (PFN_GetPrivateProfileStringA)pfnTarget;
}

static void EnsureIniExists(void)
{
    /* Build EXE directory and full INI path */
    GetModuleFileNameA(NULL, g_exeDir, MAX_PATH);
    char *lastSlash = strrchr(g_exeDir, '\\');
    if (lastSlash) lastSlash[1] = '\0';

    snprintf(g_iniFullPath, MAX_PATH, "%sTonka.ini", g_exeDir);

    /* If Tonka.ini doesn't exist, create it with a saves directory */
    if (GetFileAttributesA(g_iniFullPath) == INVALID_FILE_ATTRIBUTES) {
        char savesDir[MAX_PATH];
        snprintf(savesDir, MAX_PATH, "%ssaves", g_exeDir);
        CreateDirectoryA(savesDir, NULL);

        HANDLE hFile = CreateFileA(g_iniFullPath, GENERIC_WRITE, 0, NULL,
                                   CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            char buf[512];
            int len = snprintf(buf, sizeof(buf),
                "[Directories]\r\nSaveDir=%s\r\n", savesDir);
            DWORD written;
            WriteFile(hFile, buf, len, &written, NULL);
            CloseHandle(hFile);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  WinG API — fully implemented (used by TONKA.EXE)                  */
/* ------------------------------------------------------------------ */

__declspec(dllexport) HDC WINAPI WinGCreateDC(void)
{
    return CreateCompatibleDC(NULL);
}

__declspec(dllexport) HBITMAP WINAPI WinGCreateBitmap(HDC hdc,
    const BITMAPINFO *pHeader, void **ppBits)
{
    return CreateDIBSection(hdc, pHeader, DIB_RGB_COLORS, ppBits, NULL, 0);
}

__declspec(dllexport) BOOL WINAPI WinGBitBlt(HDC hdcDst, int xDst, int yDst,
    int nWidth, int nHeight, HDC hdcSrc, int xSrc, int ySrc)
{
    return BitBlt(hdcDst, xDst, yDst, nWidth, nHeight,
                  hdcSrc, xSrc, ySrc, SRCCOPY);
}

__declspec(dllexport) UINT WINAPI WinGSetDIBColorTable(HDC hdc,
    UINT iStart, UINT cEntries, const RGBQUAD *pColors)
{
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
/*  DLL entry point                                                   */
/* ------------------------------------------------------------------ */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        EnsureIniExists();
        PatchIAT();
    }
    return TRUE;
}
