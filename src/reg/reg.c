/*
 * Minimal reg.exe replacement for Windows 2000.
 * Supports: reg import <file.reg>
 * Comment random: For some reason reg command is not supported on windows 2000, this is basic tool lol to support reg command :D
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int main(int argc, char **argv)
{
    char cmd[1024];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD code = 1;

    if (argc < 3 || lstrcmpiA(argv[1], "import") != 0)
        return 1;

    wsprintfA(cmd, "regedit.exe /s \"%s\"", argv[2]);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                        NULL, NULL, &si, &pi))
        return 1;

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (int)code;
}
