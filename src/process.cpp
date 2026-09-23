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
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);

        throw RadialException("Failed to create subprocess.");
    }

    if (primary)
    {
        currentProcess = procInfo.hProcess;

        SetConsoleCtrlHandler(&handleInterrupt, true);
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

    if (!CloseHandle(startupInfo.hStdOutput) || !CloseHandle(startupInfo.hStdError) || !CloseHandle(procInfo.hProcess) || !CloseHandle(procInfo.hThread))
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

            if (!ReadFile(errHandle, buffer, 1024, &read, nullptr) || read == 0)
            {
                break;
            }

            buffer[read] = '\0';

            Utils::error(buffer);
        }
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

    if (display)
    {
        pollfd fds[2];

        fds[0].fd = out[0];
        fds[1].fd = err[0];

        fds[0].events = POLLIN;
        fds[1].events = POLLIN;

        char buffer[1025];

        while (const int ready = poll(fds, 2, -1))
        {
            if (ready == -1)
            {
                break;
            }

            if ((fds[0].revents & POLLHUP) == POLLHUP || (fds[1].revents & POLLHUP) == POLLHUP)
            {
                break;
            }

            if ((fds[0].revents & POLLIN) == POLLIN)
            {
                const ssize_t length = read(out[0], buffer, sizeof(char) * 1024);

                if (length == -1)
                {
                    throw RadialException("Failed to read from subprocess.");
                }

                buffer[length] = '\0';

                Utils::info(buffer);
            }

            if ((fds[1].revents & POLLIN) == POLLIN)
            {
                const ssize_t length = read(err[0], buffer, sizeof(char) * 1024);

                if (length == -1)
                {
                    throw RadialException("Failed to read from subprocess.");
                }

                buffer[length] = '\0';

                Utils::error(buffer);
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
