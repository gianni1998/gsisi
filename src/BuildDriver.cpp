#include "BuildDriver.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>

#include <sys/wait.h>
#include <unistd.h>

void BuildDriver::build(
    const std::filesystem::path& assemblyFile,
    const std::filesystem::path& runtimeAssemblyFile,
    const std::filesystem::path& executableFile
) const {
    if (!std::filesystem::exists(assemblyFile)) {
        throw std::runtime_error(
            "Build error: generated assembly does not exist: " +
            assemblyFile.string()
        );
    }

    if (!std::filesystem::exists(runtimeAssemblyFile)) {
        throw std::runtime_error(
            "Build error: runtime assembly does not exist: " +
            runtimeAssemblyFile.string()
        );
    }

    if (!executableFile.parent_path().empty()) {
        std::filesystem::create_directories(executableFile.parent_path());
    }

    std::filesystem::path programObject = assemblyFile;
    programObject.replace_extension(".o");

    std::filesystem::path runtimeObject = executableFile;
    runtimeObject += ".runtime.o";

    try {
        runCommand({
            "nasm", "-f", "elf64",
            assemblyFile.string(),
            "-o", programObject.string()
        });

        runCommand({
            "nasm", "-f", "elf64",
            runtimeAssemblyFile.string(),
            "-o", runtimeObject.string()
        });

        runCommand({
            "ld",
            programObject.string(),
            runtimeObject.string(),
            "-o", executableFile.string()
        });
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(programObject, ignored);
        std::filesystem::remove(runtimeObject, ignored);
        throw;
    }

    std::error_code ignored;
    std::filesystem::remove(programObject, ignored);
    std::filesystem::remove(runtimeObject, ignored);
}

void BuildDriver::runCommand(
    const std::vector<std::string>& arguments
) {
    if (arguments.empty()) {
        throw std::runtime_error("Build internal error: empty command");
    }

    std::vector<char*> argv;
    argv.reserve(arguments.size() + 1);

    for (const std::string& argument : arguments) {
        argv.push_back(const_cast<char*>(argument.c_str()));
    }

    argv.push_back(nullptr);

    const pid_t child = fork();

    if (child < 0) {
        throw std::system_error(errno, std::generic_category(), "fork failed");
    }

    if (child == 0) {
        execvp(argv[0], argv.data());
        _exit(127);
    }

    int status = 0;
    pid_t result = 0;

    do {
        result = waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        throw std::system_error(errno, std::generic_category(), "waitpid failed");
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error(
            "Build command failed: " + arguments.front()
        );
    }
}