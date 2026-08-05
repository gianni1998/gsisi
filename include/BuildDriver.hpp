#pragma once

#include <filesystem>
#include <string>
#include <vector>

class BuildDriver {
public:
    void build(
        const std::filesystem::path& assemblyFile,
        const std::filesystem::path& runtimeAssemblyFile,
        const std::filesystem::path& executableFile
    ) const;

private:
    static void runCommand(const std::vector<std::string>& arguments);
};