#include "Save/SaveService.h"

#include <fstream>
#include <sstream>

SaveService::SaveService(std::string filePath) : filePath(std::move(filePath))
{
    Load();
}

void SaveService::SetInt(const std::string& key, int value)
{
    SetString(key, std::to_string(value));
}

int SaveService::GetInt(const std::string& key, int defaultValue) const
{
    const auto it = values.find(key);
    if (it == values.end())
        return defaultValue;

    try
    {
        return std::stoi(it->second);
    }
    catch (...)
    {
        return defaultValue;
    }
}

void SaveService::SetFloat(const std::string& key, float value)
{
    SetString(key, std::to_string(value));
}

float SaveService::GetFloat(const std::string& key, float defaultValue) const
{
    const auto it = values.find(key);
    if (it == values.end())
        return defaultValue;

    try
    {
        return std::stof(it->second);
    }
    catch (...)
    {
        return defaultValue;
    }
}

void SaveService::SetString(const std::string& key, const std::string& value)
{
    values[key] = value;
    Save();
}

std::string SaveService::GetString(const std::string& key, const std::string& defaultValue) const
{
    const auto it = values.find(key);
    return it != values.end() ? it->second : defaultValue;
}

bool SaveService::HasKey(const std::string& key) const
{
    return values.count(key) > 0;
}

void SaveService::Load()
{
    values.clear();

    std::ifstream file(filePath);
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line))
    {
        const size_t separator = line.find('=');
        if (separator == std::string::npos)
            continue;

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        values[key] = value;
    }
}

void SaveService::Save() const
{
    std::ofstream file(filePath, std::ios::trunc);
    if (!file.is_open())
        return;

    for (const auto& [key, value] : values)
        file << key << "=" << value << "\n";
}
