#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "AssemblyGenerator.hpp"
#include "BuildDriver.hpp"
#include "IR.hpp"
#include "IRGenerator.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "SemanticAnalyser.hpp"
#include "Token.hpp"

namespace {

std::string readSourceFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);

    if (!input) {
        throw std::runtime_error(
            "could not open source file '" + path.string() + "'"
        );
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (input.bad()) {
        throw std::runtime_error(
            "failed while reading source file '" + path.string() + "'"
        );
    }

    return buffer.str();
}

std::filesystem::path compilerDirectory() {
    std::error_code error;
    const std::filesystem::path executable =
        std::filesystem::read_symlink("/proc/self/exe", error);

    if (error) {
        throw std::runtime_error(
            "could not locate the compiler executable: " + error.message()
        );
    }

    return executable.parent_path();
}

void writeAssemblyFile(
    const std::filesystem::path& path,
    const std::string& assembly
) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(
        path,
        std::ios::binary | std::ios::trunc
    );

    if (!output) {
        throw std::runtime_error(
            "could not open assembly file '" + path.string() + "'"
        );
    }

    output << assembly;
    output.close();

    if (!output) {
        throw std::runtime_error(
            "failed while writing assembly file '" + path.string() + "'"
        );
    }
}

} // namespace

/**
 * Usage:
 * ./si_compiler <source_file.si> <output_executable>
 */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr
            << "Usage: " << argv[0]
            << " <source_file.si> <output_executable>\n";

        return 1;
    }

    try {
        const std::filesystem::path sourcePath = argv[1];
        const std::filesystem::path executablePath = argv[2];

        std::filesystem::path assemblyPath = executablePath;
        assemblyPath += ".asm";

        const std::filesystem::path runtimePath =
            compilerDirectory() / "libsi" / "out.asm";

        Lexer lexer(readSourceFile(sourcePath));
        std::vector<Token> tokens = lexer.tokenise();

        Parser parser(std::move(tokens));
        std::unique_ptr<ProgramNode> ast = parser.parseProgram();

        SemanticAnalyser analyser;
        analyser.analyze(*ast);

        IRGenerator irGenerator;
        IRProgram ir = irGenerator.generate(*ast);

        std::ostringstream assemblyBuffer;
        AssemblyGenerator assemblyGenerator;
        assemblyGenerator.generate(ir, assemblyBuffer);

        writeAssemblyFile(assemblyPath, assemblyBuffer.str());

        BuildDriver buildDriver;
        buildDriver.build(
            assemblyPath,
            runtimePath,
            executablePath
        );

        std::cout
            << "Built executable: "
            << executablePath
            << '\n';
    }
    catch (const std::exception& error) {
        std::cerr
            << "Compilation error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}