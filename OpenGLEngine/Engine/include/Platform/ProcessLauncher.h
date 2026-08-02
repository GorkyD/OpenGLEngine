#pragma once

#include <string>

class ProcessLauncher
{
public:
    static bool IsSupported();

    static std::string GetExecutablePath();
    static bool Launch(const std::string& commandLine);
};
