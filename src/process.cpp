#include "../include/process.h"

#ifdef _WIN32

#include <Windows.h>

HANDLE currentProcess = INVALID_HANDLE_VALUE;

BOOL WINAPI handleInterrupt(DWORD control)
{
    if (currentProcess != INVALID_HANDLE_VALUE)
    {
        TerminateProcess(currentProcess, 0);
    }

    return TRUE;
}

bool readHandle(HANDLE handle, void(*output)(const std::string&))
{
    DWORD read;

    if (PeekNamedPipe(handle, nullptr, 0, nullptr, &read, nullptr) && read == 0)
    {
        return true;
    }

    char buffer[1025];

    if (!ReadFile(handle, buffer, 1024, &read, nullptr) || read == 0)
    {
        return false;
    }

    buffer[read] = '\0';

    output(buffer);

    return true;
}

int Process::run(std::string cmd, const bool primary, const bool display)
{
    STARTUPINFO startupInfo = { 0 };

    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;

    HANDLE outHandle;
    HANDLE errHandle;

    SECURITY_ATTRIBUTES attr = { 0 };

    attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    attr.bInheritHandle = true;

    if (!CreatePipe(&outHandle, &startupInfo.hStdOutput, &attr, 0))
    {
        throw RadialException("Failed to create subprocess.");
    }

    if (!CreatePipe(&errHandle, &startupInfo.hStdError, &attr, 0))
    {
        throw RadialException("Failed to create subprocess.");
    }

    PROCESS_INFORMATION procInfo;

    if (!CreateProcess(nullptr, cmd.data(), nullptr, nullptr, true, 0, nullptr, nullptr, &startupInfo, &procInfo))
    {
        CloseHandle(outHandle);
        CloseHandle(errHandle);
        CloseHandle(startupInfo.hStdOutput);
        CloseHandle(startupInfo.hStdError);
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);

        throw RadialException("Failed to create subprocess.");
    }

    if (!CloseHandle(startupInfo.hStdOutput) || !CloseHandle(startupInfo.hStdError))
    {
        throw RadialException("Failed to create subprocess.");
    }

    if (primary)
    {
        currentProcess = procInfo.hProcess;

        SetConsoleCtrlHandler(&handleInterrupt, true);
    }

    if (display)
    {
        while (true)
        {
            if (!readHandle(outHandle, Utils::info) || !readHandle(errHandle, Utils::error))
            {
                break;
            }
        }
    }

    WaitForSingleObject(procInfo.hProcess, INFINITE);

    if (primary)
    {
        SetConsoleCtrlHandler(&handleInterrupt, false);

        currentProcess = INVALID_HANDLE_VALUE;
    }

    DWORD code;

    if (!GetExitCodeProcess(procInfo.hProcess, &code))
    {
        throw RadialException("Failed to get subprocess exit code.");
    }

    if (!CloseHandle(procInfo.hProcess) || !CloseHandle(procInfo.hThread))
    {
        throw RadialException("Failed to close subprocess.");
    }

    if (display)
    {
        readHandle(outHandle, Utils::info);
        readHandle(errHandle, Utils::error);
    }

    if (!CloseHandle(outHandle) || !CloseHandle(errHandle))
    {
        throw RadialException("Failed to close subprocess.");
    }

    return code;
}

#else

#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t currentProcess = -1;

void handleInterrupt(int signal, siginfo_t* info, void* context)
{
    if (currentProcess != -1)
    {
        kill(currentProcess, SIGINT);
    }
}

bool readFile(const pollfd& poll, void(*output)(const std::string&))
{
    if ((poll.revents & POLLHUP) == POLLHUP)
    {
        return false;
    }

    if ((poll.revents & POLLIN) != POLLIN)
    {
        return true;
    }

    char buffer[1025];

    const ssize_t length = read(poll.fd, buffer, sizeof(char) * 1024);

    if (length == -1)
    {
        throw RadialException("Failed to read from subprocess.");
    }

    buffer[length] = '\0';

    output(buffer);

    return true;
}

int Process::run(std::string cmd, const bool primary, const bool display)
{
    int out[2];
    int err[2];

    if (pipe(out) || pipe(err))
    {
        throw RadialException("Failed to create subprocess.");
    }

    const pid_t id = fork();

    if (id == -1)
    {
        close(out[0]);
        close(out[1]);
        close(err[0]);
        close(err[1]);

        throw RadialException("Failed to create subprocess.");
    }

    if (!id)
    {
        if (!dup2(out[1], STDOUT_FILENO) || !dup2(err[1], STDERR_FILENO))
        {
            return errno;
        }

        if (close(out[0]) || close(err[0]))
        {
            return errno;
        }

        return execlp("sh", "sh", "-c", cmd.c_str(), NULL);
    }

    if (close(out[1]) || close(err[1]))
    {
        throw RadialException("Failed to create subprocess.");
    }

    struct sigaction prevAction;

    if (primary)
    {
        currentProcess = id;

        struct sigaction action = { 0 };

        action.sa_sigaction = &handleInterrupt;

        sigaction(SIGINT, &action, &prevAction);
    }

    pollfd fds[2];

    if (display)
    {
        fds[0].fd = out[0];
        fds[1].fd = err[0];

        fds[0].events = POLLIN;
        fds[1].events = POLLIN;

        while (poll(fds, 2, -1) > 0)
        {
            if (!readFile(fds[0], Utils::info) || !readFile(fds[1], Utils::error))
            {
                break;
            }
        }
    }

    if (close(out[0]) || close(err[0]))
    {
        throw RadialException("Failed to close subprocess.");
    }

    if (primary)
    {
        sigaction(SIGINT, &prevAction, nullptr);

        currentProcess = -1;
    }

    int code;

    if (waitpid(id, &code, 0) == -1)
    {
        throw RadialException("Failed to close subprocess.");
    }

    return code;
}

#endif
