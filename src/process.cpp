#include "../include/process.h"

#ifdef _WIN32

#include <Windows.h>

int Process::run(std::string cmd, const bool display)
{
    STARTUPINFO startupInfo = { 0 };

    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;

    HANDLE outHandle;

    SECURITY_ATTRIBUTES attr = { 0 };

    attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    attr.bInheritHandle = true;

    if (!CreatePipe(&outHandle, &startupInfo.hStdOutput, &attr, 0))
    {
        throw RadialException("Failed to create subprocess.");
    }

    PROCESS_INFORMATION procInfo;

    if (!CreateProcess(nullptr, cmd.data(), nullptr, nullptr, true, 0, nullptr, nullptr, &startupInfo, &procInfo))
    {
        CloseHandle(outHandle);
        CloseHandle(startupInfo.hStdOutput);
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);

        throw RadialException("Failed to create subprocess.");
    }

    WaitForSingleObject(procInfo.hProcess, INFINITE);

    DWORD code;

    if (!GetExitCodeProcess(procInfo.hProcess, &code))
    {
        throw RadialException("Failed to get subprocess exit code.");
    }

    if (!CloseHandle(startupInfo.hStdOutput) || !CloseHandle(procInfo.hProcess) || !CloseHandle(procInfo.hThread))
    {
        throw RadialException("Failed to close subprocess.");
    }

    if (display)
    {
        while (true)
        {
            char buffer[1025];

            DWORD read;

            if (!ReadFile(outHandle, buffer, 1024, &read, nullptr) || read == 0)
            {
                break;
            }

            buffer[read] = '\0';

            Utils::info(buffer);
        }
    }

    if (!CloseHandle(outHandle))
    {
        throw RadialException("Failed to close subprocess.");
    }

    return code;
}

#else

#include <stdio.h>

int Process::run(std::string cmd, const bool display)
{
    FILE* proc = popen((cmd + " 2>&1").c_str(), "r");

    if (!proc)
    {
        throw RadialException("Failed to create subprocess.");
    }

    if (display)
    {
        char buffer[1025];

        while (const size_t read = fread(buffer, sizeof(char), 1024, proc))
        {
            buffer[read] = '\0';

            Utils::info(buffer);
        }
    }

    return pclose(proc);
}

#endif
