#include "Platform/ProcessLauncher.h"

#ifdef _WIN32
#include <windows.h>

bool ProcessLauncher::IsSupported()
{
    return true;
}

std::string ProcessLauncher::GetExecutablePath()
{
    char path[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) == 0)
        return "";

    return path;
}

bool ProcessLauncher::Launch(const std::string& commandLine)
{
    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    std::string mutableCommandLine = commandLine;

    if (!CreateProcessA(nullptr, mutableCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
        return false;

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return true;
}
#else
#include <cstdlib>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

bool ProcessLauncher::IsSupported()
{
    return true;
}

std::string ProcessLauncher::GetExecutablePath()
{
    char path[4096] = {};

#ifdef __APPLE__
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0)
        return "";

    return path;
#else
    const ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0)
        return "";

    return std::string(path, static_cast<size_t>(length));
#endif
}

bool ProcessLauncher::Launch(const std::string& commandLine)
{
    return std::system((commandLine + " &").c_str()) == 0;
}
#endif
