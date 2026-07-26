#pragma once

#include <string>
#include <unordered_map>

class SaveService
{
public:
    explicit SaveService(std::string filePath = "save.txt");

    void SetInt(const std::string& key, int value);
    int GetInt(const std::string& key, int defaultValue = 0) const;

    void SetFloat(const std::string& key, float value);
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;

    void SetString(const std::string& key, const std::string& value);
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    bool HasKey(const std::string& key) const;

    void Load();
    void Save() const;

private:
    std::string filePath;
    std::unordered_map<std::string, std::string> values;
};
